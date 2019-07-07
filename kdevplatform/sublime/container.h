/***************************************************************************
 *   Copyright 2006-2007 Alexander Dymo  <adymo@kdevelop.org>              *
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
#ifndef KDEVPLATFORM_SUBLIMECONTAINER_H
#define KDEVPLATFORM_SUBLIMECONTAINER_H

#include <QWidget>
#include "sublimeexport.h"

class QMenu;
class QPaintEvent;

namespace Sublime {

class View;
class Document;
class ContainerPrivate;

/**
@short Container for the widgets.

This container is placed inside mainwindow splitters to show widgets
for views in the area.
*/
class KDEVPLATFORMSUBLIME_EXPORT Container: public QWidget {
Q_OBJECT
public:
    explicit Container(QWidget *parent = nullptr);
    ~Container() override;

    /**Adds the widget for given @p view to the container.*/
    void addWidget(Sublime::View* view, int position = -1);
    /**Removes the widget from the container.*/
    void removeWidget(QWidget *w);
    /** @return true if widget is placed inside this container.*/
    bool hasWidget(QWidget* w) const;

    QList<View*> views() const;

    int count() const;
    QWidget *currentWidget() const;
    void setCurrentWidget(QWidget *w);
    QWidget *widget(int i) const;
    int indexOf(QWidget *w) const;

    View *viewForWidget(QWidget *w) const;

    void setTabBarHidden(bool hide);

    void setTabColor(const View* view, const QColor& color);
    void setTabColors(const QHash<const View*, QColor>& colors);
    void resetTabColors(const QColor& color);

    /** Adds a corner widget to the left of this containers tab-bar. To remove it again, just delete it.
      * The ownership otherwise goes to the container. */
    void setLeftCornerWidget(QWidget* widget);
    void showTooltipForTab(int tab);

    bool isCurrentTab(int tab) const;
    /// @return Rect in global position of the tab identified by index @p tab
    QRect tabRect(int tab) const;

    static bool configTabBarVisible();

Q_SIGNALS:
    void activateView(Sublime::View* view);
    void requestClose(QWidget *w);
    /**
     * This signal is emitted whenever the users double clicks on the free
     * space next to the tab bar. Typically, a new document should be
     * created.
     */
    void newTabRequested();
    void tabContextMenuRequested(Sublime::View* view, QMenu* menu);
    /**
     * @p view The view represented by the tab that was hovered
     * @p Container The tab container that triggered the event
     * @p idx The index of the tab that was hovered
     */
    void tabToolTipRequested(Sublime::View* view, Sublime::Container* container, int idx);
    void tabDoubleClicked(Sublime::View* view);

private Q_SLOTS:
    void widgetActivated(int idx);
    void documentTitleChanged(Sublime::Document* doc);
    void statusIconChanged(Sublime::Document*);
    void statusChanged(Sublime::View *view);
    void requestClose(int idx);
    void tabMoved(int from, int to);
    void contextMenu(const QPoint&);
    void doubleClickTriggered(int tab);
    void documentListActionTriggered(QAction*);

private:
    Sublime::View* currentView() const;

protected:
    void focusInEvent(QFocusEvent*) override;

private:
    const QScopedPointer<class ContainerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Container)
};

}

#endif

