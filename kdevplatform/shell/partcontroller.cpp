/***************************************************************************
 *   Copyright 2006 Adam Treat  <treat@kde.org>                            *
 *   Copyright 2007 Alexander Dymo  <adymo@kdevelop.org>                   *
 *   Copyright 2015 Kevin Funk <kfunk@kde.org>                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "partcontroller.h"

#include <QAction>
#include <QMimeDatabase>
#include <QMimeType>

#include <KActionCollection>
#include <KToggleAction>
#include <KLocalizedString>
#include <KMimeTypeTrader>

#include <KParts/Part>

#include <KTextEditor/View>
#include <KTextEditor/Editor>
#include <KTextEditor/Document>

#include "core.h"
#include "textdocument.h"
#include "debug.h"
#include "uicontroller.h"
#include "mainwindow.h"
#include <interfaces/isession.h>
#include <interfaces/iuicontroller.h>
#include <interfaces/idocumentcontroller.h>
#include <sublime/area.h>

namespace KDevelop
{

class PartControllerPrivate
{
public:
    explicit PartControllerPrivate(Core* core)
        : m_core(core)
    {}

    bool m_showTextEditorStatusBar = false;
    QString m_editor;
    QStringList m_textTypes;

    Core* const m_core;
};

PartController::PartController(Core *core, QWidget *toplevel)
    : IPartController(toplevel)
    , d_ptr(new PartControllerPrivate(core))

{
    setObjectName(QStringLiteral("PartController"));

    //Cache this as it is too expensive when creating parts
    //     KConfig * config = Config::standard();
    //     config->setGroup( "General" );
    //
    //     d->m_textTypes = config->readEntry( "TextTypes", QStringList() );
    //
    //     config ->setGroup( "Editor" );
    //     d->m_editor = config->readPathEntry( "EmbeddedKTextEditor", QString() );

    // required early because some actions are checkable and need to be initialized
    loadSettings(false);

    if (!(Core::self()->setupFlags() & Core::NoUi))
        setupActions();
}

PartController::~PartController() = default;

bool PartController::showTextEditorStatusBar() const
{
    Q_D(const PartController);

    return d->m_showTextEditorStatusBar;
}

void PartController::setShowTextEditorStatusBar(bool show)
{
    Q_D(PartController);

    if (d->m_showTextEditorStatusBar == show)
        return;

    d->m_showTextEditorStatusBar = show;

    // update
    const auto areas = Core::self()->uiControllerInternal()->allAreas();
    for (Sublime::Area* area : areas) {
        const auto views = area->views();
        for (Sublime::View* view : views) {
            if (!view->hasWidget())
                continue;

            auto textView = qobject_cast<KTextEditor::View*>(view->widget());
            if (textView) {
                textView->setStatusBarEnabled(show);
            }
        }
    }

    // also notify active view that it should update the "view status"
    auto* textView = qobject_cast<TextView*>(Core::self()->uiControllerInternal()->activeSublimeWindow()->activeView());
    if (textView) {
        emit textView->statusChanged(textView);
    }
}

//MOVE BACK TO DOCUMENTCONTROLLER OR MULTIBUFFER EVENTUALLY
bool PartController::isTextType(const QMimeType& mimeType)
{
    Q_D(PartController);

    bool isTextType = false;
    if (d->m_textTypes.contains(mimeType.name()))
    {
        isTextType = true;
    }

    // is this regular text - open in editor
    return ( isTextType
             || mimeType.inherits(QStringLiteral("text/plain"))
             || mimeType.inherits(QStringLiteral("text/html"))
             || mimeType.inherits(QStringLiteral("application/x-zerosize")));
}

KTextEditor::Editor* PartController::editorPart() const
{
    return KTextEditor::Editor::instance();
}

KTextEditor::Document* PartController::createTextPart()
{
    return editorPart()->createDocument(this);
}

KParts::Part* PartController::createPart( const QString & mimeType,
        const QString & partType,
        const QString & className,
        const QString & preferredName )
{
    KPluginFactory * editorFactory = findPartFactory(
                                          mimeType,
                                          partType,
                                          preferredName );

    if ( !className.isEmpty() && editorFactory )
    {
        return editorFactory->create<KParts::Part>(
                   nullptr,
                   this,
                   className);
    }

    return nullptr;
}

bool PartController::canCreatePart(const QUrl& url)
{
    if (!url.isValid()) return false;

    QString mimeType;
    if ( url.isEmpty() )
        mimeType = QStringLiteral("text/plain");
    else
        mimeType = QMimeDatabase().mimeTypeForUrl(url).name();

    KService::List offers = KMimeTypeTrader::self()->query(
                                mimeType,
                                QStringLiteral("KParts/ReadOnlyPart") );

    return offers.count() > 0;
}

KParts::Part* PartController::createPart( const QUrl & url, const QString& preferredPart )
{
    qCDebug(SHELL) << "creating part with url" << url << "and pref part:" << preferredPart;
    QString mimeType;
    if ( url.isEmpty() )
        //create a part for empty text file
        mimeType = QStringLiteral("text/plain");
    else if ( !url.isValid() )
        return nullptr;
    else
        mimeType = QMimeDatabase().mimeTypeForUrl(url).name();

    KParts::Part* part = createPart( mimeType, preferredPart );
    if( part )
    {
        readOnly( part ) ->openUrl( url );
        return part;
    }

    return nullptr;
}

KParts::ReadOnlyPart* PartController::activeReadOnly() const
{
    return readOnly( activePart() );
}

KParts::ReadWritePart* PartController::activeReadWrite() const
{
    return readWrite( activePart() );
}

KParts::ReadOnlyPart* PartController::readOnly( KParts::Part * part ) const
{
    return qobject_cast<KParts::ReadOnlyPart*>( part );
}

KParts::ReadWritePart* PartController::readWrite( KParts::Part * part ) const
{
    return qobject_cast<KParts::ReadWritePart*>( part );
}

void PartController::loadSettings( bool projectIsLoaded )
{
    Q_D(PartController);

    Q_UNUSED( projectIsLoaded );

    KConfigGroup cg(KSharedConfig::openConfig(), "UiSettings");
    d->m_showTextEditorStatusBar = cg.readEntry("ShowTextEditorStatusBar", false);
}

void PartController::saveSettings( bool projectIsLoaded )
{
    Q_D(PartController);

    Q_UNUSED( projectIsLoaded );

    KConfigGroup cg(KSharedConfig::openConfig(), "UiSettings");
    cg.writeEntry("ShowTextEditorStatusBar", d->m_showTextEditorStatusBar);
}

void PartController::initialize()
{
}

void PartController::cleanup()
{
    saveSettings(false);
}

void PartController::setupActions()
{
    Q_D(PartController);

    KActionCollection* actionCollection =
        d->m_core->uiControllerInternal()->defaultMainWindow()->actionCollection();

    QAction* action;

    action = KStandardAction::showStatusbar(this, SLOT(setShowTextEditorStatusBar(bool)), actionCollection);
    action->setWhatsThis(i18n("Use this command to show or hide the view's statusbar"));
    action->setChecked(showTextEditorStatusBar());
}

//BEGIN KTextEditor::MdiContainer
void PartController::setActiveView(KTextEditor::View *view)
{
  Q_UNUSED(view)
  // NOTE: not implemented
}

KTextEditor::View *PartController::activeView()
{
    auto* textView = qobject_cast<TextView*>(Core::self()->uiController()->activeArea()->activeView());
    if (textView) {
        return textView->textView();
    }
    return nullptr;
}

KTextEditor::Document *PartController::createDocument()
{
  // NOTE: not implemented
  qCWarning(SHELL) << "WARNING: interface call not implemented";
  return nullptr;
}

bool PartController::closeDocument(KTextEditor::Document *doc)
{
  Q_UNUSED(doc)
  // NOTE: not implemented
  qCWarning(SHELL) << "WARNING: interface call not implemented";
  return false;
}

KTextEditor::View *PartController::createView(KTextEditor::Document *doc)
{
  Q_UNUSED(doc)
  // NOTE: not implemented
  qCWarning(SHELL) << "WARNING: interface call not implemented";
  return nullptr;
}

bool PartController::closeView(KTextEditor::View *view)
{
  Q_UNUSED(view)
  // NOTE: not implemented
  qCWarning(SHELL) << "WARNING: interface call not implemented";
  return false;
}
//END KTextEditor::MdiContainer


}

