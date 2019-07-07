/*
 * GDB Debugger Support
 *
 * Copyright 1999-2001 John Birch <jbb@kdevelop.org>
 * Copyright 2001 by Bernd Gehrmann <bernd@kdevelop.org>
 * Copyright 2006 Vladimir Prus <ghost@cs.msu.su>
 * Copyright 2007 Hamish Rodda <rodda@kde.org>
 * Copyright 2009 Niko Sams <niko.sams@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "debugsession.h"

#include "debuglog.h"
#include "debuggerplugin.h"
#include "gdb.h"
#include "gdbbreakpointcontroller.h"
#include "gdbframestackmodel.h"
#include "mi/micommand.h"
#include "stty.h"
#include "variablecontroller.h"

#include <debugger/breakpoint/breakpoint.h>
#include <debugger/breakpoint/breakpointmodel.h>
#include <execute/iexecuteplugin.h>
#include <interfaces/icore.h>
#include <interfaces/idebugcontroller.h>
#include <interfaces/ilaunchconfiguration.h>
#include <util/environmentprofilelist.h>

#include <KLocalizedString>
#include <KMessageBox>
#include <KShell>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QGuiApplication>

using namespace KDevMI::GDB;
using namespace KDevMI::MI;
using namespace KDevelop;

DebugSession::DebugSession(CppDebuggerPlugin *plugin)
    : MIDebugSession(plugin)
{
    m_breakpointController = new BreakpointController(this);
    m_variableController = new VariableController(this);
    m_frameStackModel = new GdbFrameStackModel(this);

    if (m_plugin) m_plugin->setupToolViews();
}

DebugSession::~DebugSession()
{
    if (m_plugin) m_plugin->unloadToolViews();
}

void DebugSession::setAutoDisableASLR(bool enable)
{
    m_autoDisableASLR = enable;
}

BreakpointController *DebugSession::breakpointController() const
{
    return m_breakpointController;
}

VariableController *DebugSession::variableController() const
{
    return m_variableController;
}

GdbFrameStackModel *DebugSession::frameStackModel() const
{
    return m_frameStackModel;
}

GdbDebugger *DebugSession::createDebugger() const
{
    return new GdbDebugger;
}

void DebugSession::initializeDebugger()
{
    //addCommand(new GDBCommand(GDBMI::EnableTimings, "yes"));

    addCommand(new CliCommand(MI::GdbShow, QStringLiteral("version"), this, &DebugSession::handleVersion));

    // This makes gdb pump a variable out on one line.
    addCommand(MI::GdbSet, QStringLiteral("width 0"));
    addCommand(MI::GdbSet, QStringLiteral("height 0"));

    addCommand(MI::SignalHandle, QStringLiteral("SIG32 pass nostop noprint"));
    addCommand(MI::SignalHandle, QStringLiteral("SIG41 pass nostop noprint"));
    addCommand(MI::SignalHandle, QStringLiteral("SIG42 pass nostop noprint"));
    addCommand(MI::SignalHandle, QStringLiteral("SIG43 pass nostop noprint"));

    addCommand(MI::EnablePrettyPrinting);

    addCommand(MI::GdbSet, QStringLiteral("charset UTF-8"));
    addCommand(MI::GdbSet, QStringLiteral("print sevenbit-strings off"));

    QString fileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                              QStringLiteral("kdevgdb/printers/gdbinit"));
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        QString quotedPrintersPath = fileInfo.dir().path()
                                             .replace(QLatin1Char('\\'), QLatin1String("\\\\"))
                                             .replace(QLatin1Char('"'), QLatin1String("\\\""));
        addCommand(MI::NonMI,
                   QStringLiteral("python sys.path.insert(0, \"%0\")").arg(quotedPrintersPath));
        addCommand(MI::NonMI, QLatin1String("source ") + fileName);
    }

    // GDB can't disable ASLR on CI server.
    addCommand(MI::GdbSet,
               QStringLiteral("disable-randomization %1").arg(m_autoDisableASLR ? QLatin1String("on") : QLatin1String("off")));

    qCDebug(DEBUGGERGDB) << "Initialized GDB";
}

void DebugSession::configInferior(ILaunchConfiguration *cfg, IExecutePlugin *iexec, const QString &)
{
    // Read Configuration values
    KConfigGroup grp = cfg->config();
    bool breakOnStart = grp.readEntry(KDevMI::Config::BreakOnStartEntry, false);
    bool displayStaticMembers = grp.readEntry(Config::StaticMembersEntry, false);
    bool asmDemangle = grp.readEntry(Config::DemangleNamesEntry, true);

    if (breakOnStart) {
        BreakpointModel* m = ICore::self()->debugController()->breakpointModel();
        bool found = false;
        const auto breakpoints = m->breakpoints();
        for (Breakpoint* b : breakpoints) {
            if (b->location() == QLatin1String("main")) {
                found = true;
                break;
            }
        }
        if (!found) {
            m->addCodeBreakpoint(QStringLiteral("main"));
        }
    }
    // Needed so that breakpoint widget has a chance to insert breakpoints.
    // FIXME: a bit hacky, as we're really not ready for new commands.
    setDebuggerStateOn(s_dbgBusy);
    raiseEvent(debugger_ready);

    if (displayStaticMembers) {
        addCommand(MI::GdbSet, QStringLiteral("print static-members on"));
    } else {
        addCommand(MI::GdbSet, QStringLiteral("print static-members off"));
    }

    if (asmDemangle) {
        addCommand(MI::GdbSet, QStringLiteral("print asm-demangle on"));
    } else {
        addCommand(MI::GdbSet, QStringLiteral("print asm-demangle off"));
    }

    // Set the environment variables
    const EnvironmentProfileList environmentProfiles(KSharedConfig::openConfig());
    QString envProfileName = iexec->environmentProfileName(cfg);
    if (envProfileName.isEmpty()) {
        qCWarning(DEBUGGERGDB) << i18n("No environment profile specified, looks like a broken "
                                       "configuration, please check run configuration '%1'. "
                                       "Using default environment profile.", cfg->name());
        envProfileName = environmentProfiles.defaultProfileName();
    }
    const auto& envvars = environmentProfiles.createEnvironment(envProfileName, {});
    for (const auto& envvar : envvars) {
        addCommand(GdbSet, QLatin1String("environment ") + envvar);
    }

    qCDebug(DEBUGGERGDB) << "Per inferior configuration done";
}

bool DebugSession::execInferior(KDevelop::ILaunchConfiguration *cfg, IExecutePlugin *, const QString &executable)
{
    qCDebug(DEBUGGERGDB) << "Executing inferior";

    KConfigGroup grp = cfg->config();
    QUrl configGdbScript = grp.readEntry(Config::RemoteGdbConfigEntry, QUrl());
    QUrl runShellScript = grp.readEntry(Config::RemoteGdbShellEntry, QUrl());
    QUrl runGdbScript = grp.readEntry(Config::RemoteGdbRunEntry, QUrl());

    // handle remote debug
    if (configGdbScript.isValid()) {
        addCommand(MI::NonMI, QLatin1String("source ") + KShell::quoteArg(configGdbScript.toLocalFile()));
    }

    // FIXME: have a check box option that controls remote debugging
    if (runShellScript.isValid()) {
        // Special for remote debug, the remote inferior is started by this shell script
        QByteArray tty(m_tty->getSlave().toLatin1());
        QByteArray options = QByteArray(">") + tty + QByteArray("  2>&1 <") + tty;

        auto *proc = new QProcess;
        const QStringList arguments{
            QStringLiteral("-c"),
            KShell::quoteArg(runShellScript.toLocalFile()) + QLatin1Char(' ') + KShell::quoteArg(executable) + QString::fromLatin1(options),
        };

        qCDebug(DEBUGGERGDB) << "starting sh" << arguments;
        proc->start(QStringLiteral("sh"), arguments);
        //PORTING TODO QProcess::DontCare);
    }

    if (runGdbScript.isValid()) {
        // Special for remote debug, gdb script at run is requested, to connect to remote inferior

        // Race notice: wait for the remote gdbserver/executable
        // - but that might be an issue for this script to handle...

        // Note: script could contain "run" or "continue"

        // Future: the shell script should be able to pass info (like pid)
        // to the gdb script...

        addCommand(new SentinelCommand([this, runGdbScript]() {
            breakpointController()->initSendBreakpoints();

            breakpointController()->setDeleteDuplicateBreakpoints(true);
            qCDebug(DEBUGGERGDB) << "Running gdb script " << KShell::quoteArg(runGdbScript.toLocalFile());

            addCommand(MI::NonMI, QLatin1String("source ") + KShell::quoteArg(runGdbScript.toLocalFile()),
                       [this](const MI::ResultRecord&) {
                           breakpointController()->setDeleteDuplicateBreakpoints(false);
                       },
                       CmdMaybeStartsRunning);
            raiseEvent(connected_to_program);
        }, CmdMaybeStartsRunning));
    } else {
        // normal local debugging
        addCommand(MI::FileExecAndSymbols, KShell::quoteArg(executable),
                   this, &DebugSession::handleFileExecAndSymbols,
                   CmdHandlesError);
        raiseEvent(connected_to_program);

        addCommand(new SentinelCommand([this]() {
            breakpointController()->initSendBreakpoints();
            addCommand(MI::ExecRun, QString(), CmdMaybeStartsRunning);
        }, CmdMaybeStartsRunning));
    }
    return true;
}

bool DebugSession::loadCoreFile(KDevelop::ILaunchConfiguration*,
                                const QString& debugee, const QString& corefile)
{
    addCommand(MI::FileExecAndSymbols, debugee,
               this, &DebugSession::handleFileExecAndSymbols,
               CmdHandlesError);
    raiseEvent(connected_to_program);

    addCommand(NonMI, QLatin1String("core ") + corefile,
               this, &DebugSession::handleCoreFile,
               CmdHandlesError);
    return true;
}

void DebugSession::handleVersion(const QStringList& s)
{
    qCDebug(DEBUGGERGDB) << s.first();
    // minimal version is 7.0,0
    QRegExp rx(QStringLiteral("([7-9]+)\\.([0-9]+)(\\.([0-9]+))?"));
    int idx = rx.indexIn(s.first());
    if (idx == -1)
    {
        if (!qobject_cast<QGuiApplication*>(qApp))  {
            //for unittest
            qFatal("You need a graphical application.");
        }

        KMessageBox::error(
            qApp->activeWindow(),
            i18n("<b>You need gdb 7.0.0 or higher.</b><br />"
            "You are using: %1", s.first()),
            i18n("gdb error"));
        stopDebugger();
    }
}

void DebugSession::handleFileExecAndSymbols(const ResultRecord& r)
{
    if (r.reason == QLatin1String("error")) {
        KMessageBox::error(
            qApp->activeWindow(),
            i18n("<b>Could not start debugger:</b><br />")+
            r[QStringLiteral("msg")].literal(),
            i18n("Startup error"));
        stopDebugger();
    }
}

void DebugSession::handleCoreFile(const ResultRecord& r)
{
    if (r.reason != QLatin1String("error")) {
        setDebuggerStateOn(s_programExited | s_core);
    } else {
        KMessageBox::error(
            qApp->activeWindow(),
            i18n("<b>Failed to load core file</b>"
                 "<p>Debugger reported the following error:"
                 "<p><tt>%1",
            r[QStringLiteral("msg")].literal()),
            i18n("Startup error"));
        stopDebugger();
    }
}
