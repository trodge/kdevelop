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
#ifndef __KDEVPARTCONTROLLER_H__
#define __KDEVPARTCONTROLLER_H__

#include <interfaces/ipartcontroller.h>

#include <QUrl>

#include <KSharedConfig>

#include "core.h"

class QWidget;

namespace KParts
{
class Part;
class ReadOnlyPart;
class ReadWritePart;
}

namespace KTextEditor
{
class Document;
class Editor;
class View;
}

class QMimeType;

Q_DECLARE_METATYPE(KSharedConfigPtr)

namespace KDevelop
{
class PartControllerPrivate;

class KDEVPLATFORMSHELL_EXPORT PartController : public IPartController
{
    friend class Core;
    friend class CorePrivate;
    Q_OBJECT

public:
    PartController(Core *core, QWidget *toplevel);
    ~PartController() override;

    bool showTextEditorStatusBar() const;

    KTextEditor::Document* createTextPart();
    KTextEditor::Editor* editorPart() const override;

    bool canCreatePart( const QUrl &url );

    using IPartController::createPart;

    KParts::Part* createPart( const QUrl &url, const QString& prefName = QString() );
    KParts::Part* createPart( const QString &mimeType,
                              const QString &partType,
                              const QString &className,
                              const QString &preferredName = QString() );

    KParts::ReadOnlyPart* activeReadOnly() const;
    KParts::ReadWritePart* activeReadWrite() const;
    KParts::ReadOnlyPart* readOnly( KParts::Part *part ) const;
    KParts::ReadWritePart* readWrite( KParts::Part *part ) const;

    bool isTextType(const QMimeType& mimeType);

    virtual void setActiveView( KTextEditor::View * view );
    virtual KTextEditor::View * activeView();
    virtual KTextEditor::Document * createDocument();
    virtual bool closeDocument( KTextEditor::Document * doc );
    virtual KTextEditor::View * createView( KTextEditor::Document * doc );
    virtual bool closeView( KTextEditor::View * view );

public Q_SLOTS:
    void setShowTextEditorStatusBar(bool show);

protected:
    virtual void loadSettings( bool projectIsLoaded );
    virtual void saveSettings( bool projectIsLoaded );
    virtual void initialize();
    virtual void cleanup();

private:
    void setupActions();

private:
    const QScopedPointer<class PartControllerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(PartController)
};

}
#endif

