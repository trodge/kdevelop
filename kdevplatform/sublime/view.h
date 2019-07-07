/***************************************************************************
 *   Copyright 2006-2007 Alexander Dymo  <adymo@kdevelop.org>       *
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
#ifndef KDEVPLATFORM_SUBLIMEVIEW_H
#define KDEVPLATFORM_SUBLIMEVIEW_H

#include <QObject>
#include <QMetaType>

#include "sublimeexport.h"

class KConfigGroup;

class QAction;

namespace Sublime {

class Document;
class ViewPrivate;

/**
@short View - the wrapper to the widget that knows about its document

Views are the convenient way to manage a widget. It is specifically designed to be
light and fast. Use @ref Document::createView() to get the new view for the document
and call @ref View::widget() to create and get the actual widget.

It is not possible to create a view by hand. You need either subclass it or use a Document.

If you create a subclass of View you need to override Sublime::View::createWidget to
provide a custom widget for your view.

*/
class KDEVPLATFORMSUBLIME_EXPORT View: public QObject {
    Q_OBJECT
public:
    enum WidgetOwnership {
        TakeOwnership,
        DoNotTakeOwnerShip
    };
    ~View() override;

    /**@return the toolbar actions for this view, this needs to be called _after_ the first call to widget() */
    QList<QAction*> toolBarActions() const;

    /**@return the toolbar actions for this view, this needs to be called _after_ the first call to widget() */
    QList<QAction*> contextMenuActions() const;

    /**@return the document for this view.*/
    Document *document() const;
    /**@return widget for this view (creates it if it's not yet created).*/
    QWidget *widget(QWidget *parent = nullptr);
    /**@return true if this view has an initialized widget.*/
    bool hasWidget() const;

    /// Retrieve information to be placed in the status bar.
    virtual QString viewStatus() const;

    /**
     * Read session settings from the given \p config.
     *
     * The default implementation is a no-op
     *
     * @see KTextEditor::View::readSessionConfig()
     */
    virtual void readSessionConfig(KConfigGroup &config);
    /**
     * Write session settings to the \p config.
     *
     * The default implementation is a no-op
     *
     * @see KTextEditor::View::writeSessionConfig()
     */
    virtual void writeSessionConfig(KConfigGroup &config);

    void notifyPositionChanged(int newPositionInArea);

Q_SIGNALS:
    void raise(Sublime::View*);
    /// Notify that the status for this document has changed
    void statusChanged(Sublime::View*);
    void positionChanged(Sublime::View*, int);

public Q_SLOTS:
    void requestRaise();

protected:
    explicit View(Document *doc, WidgetOwnership ws = DoNotTakeOwnerShip );
    /**
     * override this function to create a custom widget in your View subclass
     * @param parent the parent widget
     * @returns a new widget which is used for this view
     */
    virtual QWidget *createWidget(QWidget *parent);

private:
    //copy is not allowed, create a new view from the document instead
    View(const View &v);
    const QScopedPointer<class ViewPrivate> d_ptr;
    Q_DECLARE_PRIVATE(View)

    friend class Document;
};

}

Q_DECLARE_METATYPE(Sublime::View*)

#endif

