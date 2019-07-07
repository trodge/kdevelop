/***************************************************************************
 *   Copyright 2007 Alexander Dymo  <adymo@kdevelop.org>            *
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
#ifndef KDEVPLATFORM_UICONTROLLER_H
#define KDEVPLATFORM_UICONTROLLER_H

#include "shellexport.h"

#include <KSharedConfig>

#include <interfaces/iuicontroller.h>
#include <interfaces/iassistant.h>
#include <sublime/controller.h>

class QListWidgetItem;

namespace Sublime {
    class ToolDocument;
}

namespace KDevelop {

class Core;
class MainWindow;
class UiControllerPrivate;

class KDEVPLATFORMSHELL_EXPORT UiController: public Sublime::Controller, public IUiController
{
    Q_OBJECT

public:
    explicit UiController(Core *core);
    ~UiController() override;

    /** @return area for currently active sublime mainwindow or 0 if
    no sublime mainwindow is active.*/
    Sublime::Area *activeArea() override;
    /** @return active sublime mainwindow or 0 if no such mainwindow is active.*/
    virtual Sublime::MainWindow *activeSublimeWindow();
    /** @return active sublime mainwindow or 0 if no such mainwindow is active.*/
    KParts::MainWindow *activeMainWindow() override;

    /** @return default main window - the main window for default area in the shell.
    No guarantee is given that it always exists so this method may return 0.*/
    MainWindow *defaultMainWindow();

    void switchToArea(const QString &areaName, SwitchMode switchMode) override;

    void addToolView(const QString &name, IToolViewFactory *factory, FindFlags state) override;
    void removeToolView(IToolViewFactory *factory) override;

    QWidget* findToolView(const QString& name, IToolViewFactory *factory, FindFlags flags) override;
    void raiseToolView(QWidget* toolViewWidget) override;

    void selectNewToolViewToAdd(MainWindow *mw);

    void initialize();
    void cleanup();

    void showSettingsDialog();
    Sublime::Controller* controller() override;

    void mainWindowAdded(Sublime::MainWindow* mainWindow);

    void saveAllAreas(const KSharedConfigPtr& config);
    void saveArea(Sublime::Area* area, KConfigGroup & group);
    void loadAllAreas(const KSharedConfigPtr& config);
    void loadArea(Sublime::Area* area, const KConfigGroup & group);

    /*! @p status must implement KDevelop::IStatus */
    void registerStatus(QObject* status) override;

    void showErrorMessage(const QString& message, int timeout) override;

    /// Returns list of available view factories together with their ToolDocuments.
    /// @see addToolView(), removeToolView(), findToolView()
    const QHash<IToolViewFactory*, Sublime::ToolDocument*>& factoryDocuments() const;

    /// Adds a tool view in the active area to the dock area @p area.
    /// @see activeArea()
    void addToolViewToDockArea(KDevelop::IToolViewFactory* factory, Qt::DockWidgetArea area);

    bool toolViewPresent(Sublime::ToolDocument* doc, Sublime::Area* area);

    QWidget* activeToolViewActionListener() const override;

    QList<Sublime::Area*> allAreas() const override;

public Q_SLOTS:
    void raiseToolView(Sublime::View * view);

private Q_SLOTS:
    void addNewToolView(MainWindow* mw, QListWidgetItem* item);

    void slotAreaChanged(Sublime::Area* area);
    void slotActiveToolViewChanged(Sublime::View* view);

private:
    void addToolViewIfWanted(IToolViewFactory* factory,
                           Sublime::ToolDocument* doc,
                           Sublime::Area* area);
    Sublime::View* addToolViewToArea(IToolViewFactory* factory,
                           Sublime::ToolDocument* doc,
                           Sublime::Area* area, Sublime::Position p=Sublime::AllPositions);
    void setupActions();

private:
    const QScopedPointer<class UiControllerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(UiController)
    friend class UiControllerPrivate;
};

}

#endif

