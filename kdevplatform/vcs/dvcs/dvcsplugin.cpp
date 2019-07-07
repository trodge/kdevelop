/***************************************************************************
 *   This file was partly taken from KDevelop's cvs plugin                 *
 *   Copyright 2007 Robert Gruber <rgruber@users.sourceforge.net>          *
 *                                                                         *
 *   Adapted for DVCS (added templates)                                    *
 *   Copyright 2008 Evgeniy Ivanov <powerfox@kde.ru>                       *
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

#ifndef DVCS_PLUGIN_CC
#define DVCS_PLUGIN_CC

#include "dvcsplugin.h"

#include <QMenu>
#include <QFileInfo>
#include <QString>

#include <KLocalizedString>
#include <KParts/MainWindow>

#include <interfaces/iuicontroller.h>
#include <interfaces/icore.h>
#include <interfaces/context.h>
#include <interfaces/contextmenuextension.h>
#include <interfaces/idocumentcontroller.h>
#include <util/scopeddialog.h>

#include "dvcsjob.h"
#include "ui/dvcsimportmetadatawidget.h"
#include "ui/branchmanager.h"
#include <vcs/vcspluginhelper.h>

namespace KDevelop
{

class DistributedVersionControlPluginPrivate
{
public:
    explicit DistributedVersionControlPluginPrivate(DistributedVersionControlPlugin * pThis)
            : m_common(new VcsPluginHelper(pThis, pThis)) {}

    ~DistributedVersionControlPluginPrivate() { delete m_common; }
    VcsPluginHelper* m_common;
};

//class DistributedVersionControlPlugin
DistributedVersionControlPlugin::DistributedVersionControlPlugin(QObject *parent, const QString& componentName)
        : IPlugin(componentName, parent)
        , d_ptr(new DistributedVersionControlPluginPrivate(this))
{}

DistributedVersionControlPlugin::~DistributedVersionControlPlugin()
{
    //TODO: Find out why this crashes on the svn tests delete d->m_factory;
}

// End:  KDevelop::IBasicVersionControl


// Begin:  KDevelop::IDistributedVersionControl

// End:  KDevelop::IDistributedVersionControl


KDevelop::VcsImportMetadataWidget*
DistributedVersionControlPlugin::createImportMetadataWidget(QWidget* parent)
{
    return new DvcsImportMetadataWidget(parent);
}

KDevelop::ContextMenuExtension
DistributedVersionControlPlugin::contextMenuExtension(Context* context, QWidget* parent)
{
    Q_D(DistributedVersionControlPlugin);

    d->m_common->setupFromContext(context);
    QList<QUrl> const & ctxUrlList = d->m_common->contextUrlList();

    bool isWorkingDirectory = false;
    for (const QUrl& url : ctxUrlList) {
        if (isValidDirectory(url)) {
            isWorkingDirectory = true;
            break;
        }
    }

    if (!isWorkingDirectory) {  // Not part of a repository
        return ContextMenuExtension();
    }

    QMenu * menu = d->m_common->commonActions(parent);
    menu->addSeparator();
    menu->addAction(i18n("Branches..."), this, SLOT(ctxBranchManager()))->setEnabled(ctxUrlList.count()==1);
    additionalMenuEntries(menu, ctxUrlList);

    ContextMenuExtension menuExt;
    menuExt.addAction(ContextMenuExtension::VcsGroup, menu->menuAction());

    return menuExt;

}

void DistributedVersionControlPlugin::additionalMenuEntries(QMenu* /*menu*/, const QList<QUrl>& /*urls*/)
{}

static QString stripPathToDir(const QString &path)
{
    QFileInfo info = QFileInfo(path);
    return info.isDir() ? info.absoluteFilePath() : info.absolutePath();
}

void DistributedVersionControlPlugin::ctxBranchManager()
{
    Q_D(DistributedVersionControlPlugin);

    QList<QUrl> const & ctxUrlList = d->m_common->contextUrlList();
    Q_ASSERT(!ctxUrlList.isEmpty());    
    
    ICore::self()->documentController()->saveAllDocuments();

    ScopedDialog<BranchManager> branchManager(stripPathToDir(ctxUrlList.front().toLocalFile()),
                                               this, core()->uiController()->activeMainWindow());
    branchManager->exec();
}

}

#endif
