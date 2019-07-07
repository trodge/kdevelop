/***************************************************************************
 *   This file was partly taken from KDevelop's cvs plugin                 *
 *   Copyright 2002-2003 Christian Loose <christian.loose@hamburg.de>      *
 *   Copyright 2007 Robert Gruber <rgruber@users.sourceforge.net>          *
 *                                                                         *
 *   Adapted for DVCS                                                      *
 *   Copyright 2008 Evgeniy Ivanov <powerfox@kde.ru>                       *
 *   Copyright Aleix Pol Gonzalez <aleixpol@kde.org>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "dvcsjob.h"
#include "debug.h"

#include <QFile>
#include <QList>
#include <QStringList>
#include <QDir>
#include <QUrl>

#include <KLocalizedString>
#include <KShell>

#include <interfaces/iplugin.h>
#include <outputview/outputmodel.h>

using namespace KDevelop;

class KDevelop::DVcsJobPrivate
{
public:
    DVcsJobPrivate() : childproc(new KProcess)
    {}

    ~DVcsJobPrivate() {
        delete childproc;
    }

    KProcess*   childproc;
    VcsJob::JobStatus status;
    QByteArray  output;
    QByteArray  errorOutput;
    IPlugin* vcsplugin = nullptr;

    QVariant results;
    OutputModel* model;

    bool ignoreError = false;
};

DVcsJob::DVcsJob(const QDir& workingDir, IPlugin* parent, OutputJob::OutputJobVerbosity verbosity)
    : VcsJob(parent, verbosity)
    , d_ptr(new DVcsJobPrivate)
{
    Q_D(DVcsJob);

    Q_ASSERT(workingDir.exists());
    d->status = JobNotStarted;
    d->vcsplugin = parent;
    d->childproc->setWorkingDirectory(workingDir.absolutePath());
    d->model = new OutputModel;
    d->ignoreError = false;
    setModel(d->model);
    setCapabilities(Killable);

    connect(d->childproc, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DVcsJob::slotProcessExited);
    connect(d->childproc, &QProcess::errorOccurred,
            this, &DVcsJob::slotProcessError);

    connect(d->childproc, &KProcess::readyReadStandardOutput,
                this, &DVcsJob::slotReceivedStdout);

}

DVcsJob::~DVcsJob() = default;

QDir DVcsJob::directory() const
{
    Q_D(const DVcsJob);

    return QDir(d->childproc->workingDirectory());
}

DVcsJob& DVcsJob::operator<<(const QString& arg)
{
    Q_D(DVcsJob);

    *d->childproc << arg;
    return *this;
}

DVcsJob& DVcsJob::operator<<(const char* arg)
{
    Q_D(DVcsJob);

    *d->childproc << QString::fromUtf8(arg);
    return *this;
}

DVcsJob& DVcsJob::operator<<(const QStringList& args)
{
    Q_D(DVcsJob);

    *d->childproc << args;
    return *this;
}

QStringList DVcsJob::dvcsCommand() const
{
    Q_D(const DVcsJob);

    return d->childproc->program();
}

QString DVcsJob::output() const
{
    Q_D(const DVcsJob);

    QByteArray stdoutbuf = rawOutput();
    int endpos = stdoutbuf.size();
    if (d->status==JobRunning) {    // We may have received only part of a code-point. apol: ASSERT?
        endpos = stdoutbuf.lastIndexOf('\n')+1; // Include the final newline or become 0, when there is no newline
    }

    return QString::fromLocal8Bit(stdoutbuf.constData(), endpos);
}

QByteArray DVcsJob::rawOutput() const
{
    Q_D(const DVcsJob);

    return d->output;
}

QByteArray DVcsJob::errorOutput() const
{
    Q_D(const DVcsJob);

    return d->errorOutput;
}

void DVcsJob::setIgnoreError(bool ignore)
{
    Q_D(DVcsJob);

    d->ignoreError = ignore;
}

void DVcsJob::setResults(const QVariant &res)
{
    Q_D(DVcsJob);

    d->results = res;
}

QVariant DVcsJob::fetchResults()
{
    Q_D(DVcsJob);

    return d->results;
}

void DVcsJob::start()
{
    Q_D(DVcsJob);

    Q_ASSERT_X(d->status != JobRunning, "DVCSjob::start", "Another process was started using this job class");

    const QDir& workingdir = directory();
    if( !workingdir.exists() ) {
        QString error = i18n( "Working Directory does not exist: %1", d->childproc->workingDirectory() );
        d->model->appendLine(error);
        setError( 255 );
        setErrorText(error);
        d->status = JobFailed;
        emitResult();
        return;
    }
    if( !workingdir.isAbsolute() ) {
        QString error = i18n( "Working Directory is not absolute: %1", d->childproc->workingDirectory() );
        d->model->appendLine(error);
        setError( 255 );
        setErrorText(error);
        d->status = JobFailed;
        emitResult();
        return;
    }

    QString commandDisplay = KShell::joinArgs(dvcsCommand());
    qCDebug(VCS) << "Execute dvcs command:" << commandDisplay;

    QString service;
    if(d->vcsplugin)
        service = d->vcsplugin->objectName();
    setObjectName(service + QLatin1String(": ") + commandDisplay);

    d->status = JobRunning;
    d->childproc->setOutputChannelMode(KProcess::SeparateChannels);
    //the started() and error() signals may be delayed! It causes crash with deferred deletion!!!
    d->childproc->start();

    d->model->appendLine(directory().path() + QLatin1String("> ") + commandDisplay);
}

void DVcsJob::setCommunicationMode(KProcess::OutputChannelMode comm)
{
    Q_D(DVcsJob);

    d->childproc->setOutputChannelMode(comm);
}

void DVcsJob::cancel()
{
    Q_D(DVcsJob);

    d->childproc->kill();
}

void DVcsJob::slotProcessError( QProcess::ProcessError err )
{
    Q_D(DVcsJob);

    d->status = JobFailed;

    setError(OutputJob::FailedShownError); //we don't want to trigger a message box

    d->errorOutput = d->childproc->readAllStandardError();

    QString displayCommand = KShell::joinArgs(dvcsCommand());
    QString completeErrorText = i18n("Process '%1' exited with status %2\n%3", displayCommand, d->childproc->exitCode(), QString::fromLocal8Bit(d->errorOutput) );
    setErrorText( completeErrorText );

    QString errorValue;
    //if trolls add Q_ENUMS for QProcess, then we can use better solution than switch:
    //QMetaObject::indexOfEnumerator(char*), QQStringLiteral(QMetaEnum::valueToKey())...
    switch (err)
    {
    case QProcess::FailedToStart:
        errorValue = QStringLiteral("FailedToStart");
        break;
    case QProcess::Crashed:
        errorValue = QStringLiteral("Crashed");
        break;
    case QProcess::Timedout:
        errorValue = QStringLiteral("Timedout");
        break;
    case QProcess::WriteError:
        errorValue = QStringLiteral("WriteError");
        break;
    case QProcess::ReadError:
        errorValue = QStringLiteral("ReadError");
        break;
    case QProcess::UnknownError:
        errorValue = QStringLiteral("UnknownError");
        break;
    }
    qCDebug(VCS) << "Found an error while running" << displayCommand << ":" << errorValue
                                                     << "Exit code is:" << d->childproc->exitCode();
    qCDebug(VCS) << "Error:" << completeErrorText;
    displayOutput(QString::fromLocal8Bit(d->errorOutput));
    d->model->appendLine(i18n("Command finished with error %1.", errorValue));

    if(verbosity()==Silent) {
        setVerbosity(Verbose);
        startOutput();
    }
    emitResult();
}

void DVcsJob::slotProcessExited(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_D(DVcsJob);

    d->status = JobSucceeded;
    d->model->appendLine(i18n("Command exited with value %1.", exitCode));

    if (exitStatus == QProcess::CrashExit)
        slotProcessError(QProcess::Crashed);

    else if (exitCode != 0 && !d->ignoreError)
        slotProcessError(QProcess::UnknownError);

    else
        jobIsReady();
}

void DVcsJob::displayOutput(const QString& data)
{
    Q_D(DVcsJob);

    d->model->appendLines(data.split(QLatin1Char('\n')));
}

void DVcsJob::slotReceivedStdout()
{
    Q_D(DVcsJob);

    QByteArray output = d->childproc->readAllStandardOutput();

    // accumulate output
    d->output.append(output);

    displayOutput(QString::fromLocal8Bit(output));
}

VcsJob::JobStatus DVcsJob::status() const
{
    Q_D(const DVcsJob);

    return d->status;
}

IPlugin* DVcsJob::vcsPlugin() const
{
    Q_D(const DVcsJob);

    return d->vcsplugin;
}

DVcsJob& DVcsJob::operator<<(const QUrl& url)
{
    Q_D(DVcsJob);

    *d->childproc << url.toLocalFile();
    return *this;
}

DVcsJob& DVcsJob::operator<<(const QList< QUrl >& urls)
{
    for (const QUrl& url : urls) {
        operator<<(url);
    }
    return *this;
}

bool DVcsJob::doKill()
{
    Q_D(DVcsJob);

    if (d->childproc->state() == QProcess::NotRunning) {
        return true;
    }

    static const int terminateKillTimeout = 1000; // ms
    d->childproc->terminate();
    bool terminated = d->childproc->waitForFinished( terminateKillTimeout );
    if( !terminated ) {
        d->childproc->kill();
        terminated = d->childproc->waitForFinished( terminateKillTimeout );
    }
    return terminated;
}

void DVcsJob::jobIsReady()
{
    emit readyForParsing(this); //let parsers to set status
    emitResult(); //KJob
    emit resultsReady(this); //VcsJob
}

KProcess* DVcsJob::process() const
{
    Q_D(const DVcsJob);

    return d->childproc;
}
