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
#include "uicontroller.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QMap>
#include <QPointer>
#include <QTimer>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KXMLGUIClient>

#include <sublime/area.h>
#include <sublime/view.h>
#include <sublime/tooldocument.h>
#include <sublime/holdupdates.h>

#include <interfaces/itoolviewactionlistener.h>
#include <util/scopeddialog.h>

#include "core.h"
#include "configpage.h"
#include "configdialog.h"
#include "debug.h"
#include "editorconfigpage.h"
#include "shellextension.h"
#include "plugincontroller.h"
#include "mainwindow.h"
#include "workingsetcontroller.h"
#include "workingsets/workingset.h"
#include "settings/bgpreferences.h"
#include "settings/languagepreferences.h"
#include "settings/environmentpreferences.h"
#include "settings/pluginpreferences.h"
#include "settings/projectpreferences.h"
#include "settings/sourceformattersettings.h"
#include "settings/uipreferences.h"
#include "settings/templateconfig.h"
#include "settings/analyzerspreferences.h"
#include "settings/documentationpreferences.h"
#include "settings/runtimespreferences.h"

namespace KDevelop {

class UiControllerPrivate {
public:
    UiControllerPrivate(Core* core, UiController* controller)
        : core(core)
        , areasRestored(false)
        , m_controller(controller)
    {
        if (Core::self()->workingSetControllerInternal())
            Core::self()->workingSetControllerInternal()->initializeController(m_controller);

        m_controller->connect(m_controller, &Sublime::Controller::mainWindowAdded, m_controller, &UiController::mainWindowAdded);

        QMap<QString, Sublime::Position> desired;

        desired[QStringLiteral("org.kdevelop.ClassBrowserView")] = Sublime::Left;
        desired[QStringLiteral("org.kdevelop.DocumentsView")] = Sublime::Left;
        desired[QStringLiteral("org.kdevelop.ProjectsView")] = Sublime::Left;
        desired[QStringLiteral("org.kdevelop.FileManagerView")] = Sublime::Left;
        desired[QStringLiteral("org.kdevelop.ProblemReporterView")] = Sublime::Bottom;
        desired[QStringLiteral("org.kdevelop.OutputView")] = Sublime::Bottom;
        desired[QStringLiteral("org.kdevelop.ContextBrowser")] = Sublime::Bottom;
        desired[QStringLiteral("org.kdevelop.KonsoleView")] = Sublime::Bottom;
        desired[QStringLiteral("org.kdevelop.SnippetView")] = Sublime::Right;
        desired[QStringLiteral("org.kdevelop.ExternalScriptView")] = Sublime::Right;
        Sublime::Area* a =
            new Sublime::Area(m_controller, QStringLiteral("code"), i18n("Code"));
        a->setDesiredToolViews(desired);
        a->setIconName(QStringLiteral("document-edit"));
        m_controller->addDefaultArea(a);

        desired.clear();
        desired[QStringLiteral("org.kdevelop.debugger.VariablesView")] = Sublime::Left;
        desired[QStringLiteral("org.kdevelop.debugger.BreakpointsView")] = Sublime::Bottom;
        desired[QStringLiteral("org.kdevelop.debugger.StackView")] = Sublime::Bottom;
        desired[QStringLiteral("org.kdevelop.debugger.ConsoleView")] = Sublime::Bottom;
        desired[QStringLiteral("org.kdevelop.KonsoleView")] = Sublime::Bottom;
        a = new Sublime::Area(m_controller, QStringLiteral("debug"), i18n("Debug"));
        a->setDesiredToolViews(desired);
        a->setIconName(QStringLiteral("debug-run"));
        m_controller->addDefaultArea(a);

        desired.clear();
        desired[QStringLiteral("org.kdevelop.ProjectsView")] = Sublime::Left;
        desired[QStringLiteral("org.kdevelop.PatchReview")] = Sublime::Bottom;

        a = new Sublime::Area(m_controller, QStringLiteral("review"), i18n("Review"));
        a->setDesiredToolViews(desired);
        a->setIconName(QStringLiteral("text-x-patch"));
        m_controller->addDefaultArea(a);

        if(!(Core::self()->setupFlags() & Core::NoUi))
        {
            defaultMainWindow = new MainWindow(m_controller);
            m_controller->addMainWindow(defaultMainWindow);
            activeSublimeWindow = defaultMainWindow;
        }
        else
        {
            activeSublimeWindow = defaultMainWindow = nullptr;
        }

        m_assistantTimer.setSingleShot(true);
        m_assistantTimer.setInterval(100);
    }

    void widgetChanged(QWidget*, QWidget* now)
    {
        if (now) {
            auto* win = qobject_cast<Sublime::MainWindow*>(now->window());
            if( win )
            {
                activeSublimeWindow = win;
            }
        }
    }

    Core* const core;
    QPointer<MainWindow> defaultMainWindow;

    QHash<IToolViewFactory*, Sublime::ToolDocument*> factoryDocuments;

    QPointer<Sublime::MainWindow> activeSublimeWindow;
    bool areasRestored;

    /// QWidget implementing IToolViewActionListener interface, or null
    QPointer<QWidget> activeActionListener;
    QTimer m_assistantTimer;

private:
    UiController *m_controller;
};


class UiToolViewFactory: public Sublime::ToolFactory {
public:
    explicit UiToolViewFactory(IToolViewFactory *factory): m_factory(factory) {}
    ~UiToolViewFactory() override { delete m_factory; }
    QWidget* create(Sublime::ToolDocument *doc, QWidget *parent = nullptr) override
    {
        Q_UNUSED( doc );
        return m_factory->create(parent);
    }

    QList< QAction* > contextMenuActions(QWidget* viewWidget) const override
    {
        return m_factory->contextMenuActions( viewWidget );
    }

    QList<QAction*> toolBarActions( QWidget* viewWidget ) const override
    {
        return m_factory->toolBarActions( viewWidget );
    }

    QString id() const override { return m_factory->id(); }
private:
    IToolViewFactory* const m_factory;
};


class ViewSelectorItem: public QListWidgetItem {
public:
    explicit ViewSelectorItem(const QString& text, IToolViewFactory* factory, QListWidget* parent = nullptr, int type = Type)
        : QListWidgetItem(text, parent, type)
        , factory(factory)
    {}
    IToolViewFactory* const factory;
};


class NewToolViewListWidget: public QListWidget {
    Q_OBJECT

public:
    explicit NewToolViewListWidget(MainWindow *mw, QWidget* parent = nullptr)
        :QListWidget(parent), m_mw(mw)
    {
        connect(this, &NewToolViewListWidget::doubleClicked, this, &NewToolViewListWidget::addNewToolViewByDoubleClick);
    }

Q_SIGNALS:
    void addNewToolView(MainWindow *mw, QListWidgetItem *item);

private Q_SLOTS:
    void addNewToolViewByDoubleClick(const QModelIndex& index)
    {
        QListWidgetItem *item = itemFromIndex(index);
        // Disable item so that the tool view can not be added again.
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        emit addNewToolView(m_mw, item);
    }

private:
    MainWindow* const m_mw;
};

UiController::UiController(Core *core)
    : Sublime::Controller(nullptr), IUiController()
    , d_ptr(new UiControllerPrivate(core, this))
{
    setObjectName(QStringLiteral("UiController"));

    if (!defaultMainWindow() || (Core::self()->setupFlags() & Core::NoUi))
        return;

    connect(qApp, &QApplication::focusChanged,
            this, [this] (QWidget* old, QWidget* now) { Q_D(UiController); d->widgetChanged(old, now); } );

    setupActions();
}

UiController::~UiController() = default;

void UiController::setupActions()
{
}

void UiController::mainWindowAdded(Sublime::MainWindow* mainWindow)
{
    connect(mainWindow, &MainWindow::activeToolViewChanged, this, &UiController::slotActiveToolViewChanged);
    connect(mainWindow, &MainWindow::areaChanged, this, &UiController::slotAreaChanged); // also check after area reconstruction
}

// FIXME: currently, this always create new window. Probably,
// should just rename it.
void UiController::switchToArea(const QString &areaName, SwitchMode switchMode)
{
    if (switchMode == ThisWindow) {
        showArea(areaName, activeSublimeWindow());
        return;
    }

    auto *main = new MainWindow(this);

    addMainWindow(main);
    showArea(areaName, main);
    main->initialize();

    // WTF? First, enabling this code causes crashes since we
    // try to disconnect some already-deleted action, or something.
    // Second, this code will disconnection the clients from guiFactory
    // of the previous main window. Ick!
#if 0
    //we need to add all existing guiclients to the new mainwindow
    //@todo adymo: add only ones that belong to the area (when the area code is there)
    const auto clients = oldMain->guiFactory()->clients();
    for (KXMLGUIClient *client : clients) {
        main->guiFactory()->addClient(client);
    }
#endif

    main->show();
}


QWidget* UiController::findToolView(const QString& name, IToolViewFactory *factory, FindFlags flags)
{
    Q_D(UiController);

    if(!d->areasRestored || !activeArea())
        return nullptr;

    const QList<Sublime::View*> views = activeArea()->toolViews();
    for (Sublime::View* view : views) {
        auto* doc = qobject_cast<Sublime::ToolDocument*>(view->document());
        if(doc && doc->title() == name && view->widget()) {
            if(flags & Raise)
                view->requestRaise();
            return view->widget();
        }
    }

    QWidget* ret = nullptr;

    if(flags & Create)
    {
        Sublime::ToolDocument* doc = d->factoryDocuments.value(factory);
        if(!doc) {
            doc = new Sublime::ToolDocument(name, this, new UiToolViewFactory(factory));
            d->factoryDocuments.insert(factory, doc);
        }

        Sublime::View* view = addToolViewToArea(factory, doc, activeArea());
        if(view)
            ret = view->widget();

        if(flags & Raise)
            findToolView(name, factory, Raise);
    }

    return ret;
}

void UiController::raiseToolView(QWidget* toolViewWidget)
{
    Q_D(UiController);

    if(!d->areasRestored)
        return;

    const QList<Sublime::View*> views = activeArea()->toolViews();
    for (Sublime::View* view : views) {
        if(view->widget() == toolViewWidget) {
            view->requestRaise();
            return;
        }
    }
}

void UiController::addToolView(const QString & name, IToolViewFactory *factory, FindFlags state)
{
    Q_D(UiController);

    if (!factory)
        return;

    qCDebug(SHELL) ;
    auto *doc = new Sublime::ToolDocument(name, this, new UiToolViewFactory(factory));
    d->factoryDocuments[factory] = doc;

    /* Until areas are restored, we don't know which views should be really
       added, and which not, so we just record view availability.  */
    if (d->areasRestored && state != None) {
         const auto areas = allAreas();
         for (Sublime::Area* area : areas) {
             addToolViewToArea(factory, doc, area);
         }
    }
}

void KDevelop::UiController::raiseToolView(Sublime::View * view)
{
    const auto areas = allAreas();
    for (Sublime::Area* area : areas) {
        if( area->toolViews().contains( view ) )
            area->raiseToolView( view );
    }

    slotActiveToolViewChanged(view);
}

void UiController::slotAreaChanged(Sublime::Area*)
{
    // this slot gets call if an area in *any* MainWindow changed
    // so let's first get the "active area"
    const auto area = activeSublimeWindow()->area();
    if (area) {
        // walk through shown tool views and maku sure the
        const auto shownIds = area->shownToolViews(Sublime::AllPositions);
        for (Sublime::View* toolView : qAsConst(area->toolViews())) {
            if (shownIds.contains(toolView->document()->documentSpecifier())) {
                slotActiveToolViewChanged(toolView);
            }
        }
    }
}

void UiController::slotActiveToolViewChanged(Sublime::View* view)
{
    Q_D(UiController);

    if (!view) {
        return;
    }

    // record the last active tool view action listener
    if (qobject_cast<IToolViewActionListener*>(view->widget())) {
        d->activeActionListener = view->widget();
    }
}

void KDevelop::UiController::removeToolView(IToolViewFactory *factory)
{
    Q_D(UiController);

    if (!factory)
        return;

    qCDebug(SHELL) ;
    //delete the tooldocument
    Sublime::ToolDocument *doc = d->factoryDocuments.value(factory);

    ///@todo adymo: on document deletion all its views shall be also deleted
    for (Sublime::View* view : doc->views()) {
        const auto areas = allAreas();
        for (Sublime::Area *area : areas) {
            if (area->removeToolView(view))
                view->deleteLater();
        }
    }

    d->factoryDocuments.remove(factory);
    delete doc;
}

Sublime::Area *UiController::activeArea()
{
    Sublime::MainWindow *m = activeSublimeWindow();
    if (m)
        return activeSublimeWindow()->area();
    return nullptr;
}

Sublime::MainWindow *UiController::activeSublimeWindow()
{
    Q_D(UiController);

    return d->activeSublimeWindow;
}

MainWindow *UiController::defaultMainWindow()
{
    Q_D(UiController);

    return d->defaultMainWindow;
}

void UiController::initialize()
{
    defaultMainWindow()->initialize();
}


void UiController::cleanup()
{
    for (Sublime::MainWindow* w : mainWindows()) {
        w->saveSettings();
    }
    saveAllAreas(KSharedConfig::openConfig());
}

void UiController::selectNewToolViewToAdd(MainWindow *mw)
{
    Q_D(UiController);

    if (!mw || !mw->area())
        return;

    ScopedDialog<QDialog> dia(mw);
    dia->setWindowTitle(i18n("Select Tool View to Add"));

    auto mainLayout = new QVBoxLayout(dia);

    auto *list = new NewToolViewListWidget(mw, dia);

    list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list->setSortingEnabled(true);
    for (QHash<IToolViewFactory*, Sublime::ToolDocument*>::const_iterator it = d->factoryDocuments.constBegin();
        it != d->factoryDocuments.constEnd(); ++it)
    {
        ViewSelectorItem *item = new ViewSelectorItem(it.value()->title(), it.key(), list);
        if (!item->factory->allowMultiple() && toolViewPresent(it.value(), mw->area())) {
            // Disable item if the tool view is already present.
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }
        list->addItem(item);
    }

    list->setFocus();
    connect(list, &NewToolViewListWidget::addNewToolView, this, &UiController::addNewToolView);
    mainLayout->addWidget(list);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    auto okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    dia->connect(buttonBox, &QDialogButtonBox::accepted, dia.data(), &QDialog::accept);
    dia->connect(buttonBox, &QDialogButtonBox::rejected, dia.data(), &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if (dia->exec() == QDialog::Accepted)
    {
        const auto items = list->selectedItems();
        for (QListWidgetItem* item : items) {
            addNewToolView(mw, item);
        }
    }
}

void UiController::addNewToolView(MainWindow *mw, QListWidgetItem* item)
{
    Q_D(UiController);

    auto *current = static_cast<ViewSelectorItem*>(item);
    Sublime::ToolDocument *doc = d->factoryDocuments[current->factory];
    Sublime::View *view = doc->createView();
    mw->area()->addToolView(view,
        Sublime::dockAreaToPosition(current->factory->defaultPosition()));
    current->factory->viewCreated(view);
}

void UiController::showSettingsDialog()
{
    ConfigDialog cfgDlg(activeMainWindow());

    auto editorConfigPage = new EditorConfigPage(&cfgDlg);
    auto languageConfigPage = new LanguagePreferences(&cfgDlg);
    auto analyzersPreferences = new AnalyzersPreferences(&cfgDlg);
    auto documentationPreferences = new DocumentationPreferences(&cfgDlg);
    auto runtimesPreferences = new RuntimesPreferences(&cfgDlg);
    auto templateConfig = new TemplateConfig(&cfgDlg);

    const auto configPages = QVector<KDevelop::ConfigPage*> {
        new UiPreferences(&cfgDlg),
        new PluginPreferences(&cfgDlg),
        new SourceFormatterSettings(&cfgDlg),
        new ProjectPreferences(&cfgDlg),
        new EnvironmentPreferences(QString(), &cfgDlg),
        templateConfig,
        documentationPreferences,
        analyzersPreferences,
        runtimesPreferences,
        languageConfigPage,
        editorConfigPage
    };

    for (auto page : configPages) {
        cfgDlg.appendConfigPage(page);
    }

    cfgDlg.appendSubConfigPage(languageConfigPage, new BGPreferences(&cfgDlg));

    auto addPluginPages = [&](IPlugin* plugin) {
        for (int i = 0, numPages = plugin->configPages(); i < numPages; ++i) {
            auto page = plugin->configPage(i, &cfgDlg);
            if (!page)
                continue;

            if (page->configPageType() == ConfigPage::LanguageConfigPage) {
                cfgDlg.appendSubConfigPage(languageConfigPage, page);
            } else if (page->configPageType() == ConfigPage::AnalyzerConfigPage) {
                cfgDlg.appendSubConfigPage(analyzersPreferences, page);
            } else if (page->configPageType() == ConfigPage::RuntimeConfigPage) {
                cfgDlg.appendSubConfigPage(runtimesPreferences, page);
            } else if (page->configPageType() == ConfigPage::DocumentationConfigPage) {
                cfgDlg.appendSubConfigPage(documentationPreferences, page);
            } else {
                cfgDlg.insertConfigPage(editorConfigPage, page);
            }
        }
    };

    auto plugins = ICore::self()->pluginController()->loadedPlugins();
    std::sort(plugins.begin(), plugins.end());

    for (IPlugin* plugin : qAsConst(plugins)) {
        addPluginPages(plugin);
    }

    // TODO: only load settings if a UI related page was changed?
    connect(&cfgDlg, &ConfigDialog::configSaved, activeSublimeWindow(), &Sublime::MainWindow::loadSettings);
    // make sure that pages get added whenever a new plugin is loaded (probably from the plugin selection dialog)
    // removal on plugin unload is already handled in ConfigDialog
    connect(ICore::self()->pluginController(), &IPluginController::pluginLoaded, &cfgDlg, addPluginPages);
    cfgDlg.exec();
}

Sublime::Controller* UiController::controller()
{
    return this;
}

KParts::MainWindow *UiController::activeMainWindow()
{
    return activeSublimeWindow();
}

void UiController::saveArea(Sublime::Area * area, KConfigGroup & group)
{
    area->save(group);
    if (!area->workingSet().isEmpty()) {
        WorkingSet* set = Core::self()->workingSetControllerInternal()->workingSet(area->workingSet());
        set->saveFromArea(area, area->rootIndex());
    }
}

void UiController::loadArea(Sublime::Area * area, const KConfigGroup & group)
{
    area->load(group);
    if (!area->workingSet().isEmpty()) {
        WorkingSet* set = Core::self()->workingSetControllerInternal()->workingSet(area->workingSet());
        Q_ASSERT(set->isConnected(area));
        Q_UNUSED(set);
    }
}

void UiController::saveAllAreas(const KSharedConfigPtr& config)
{
    KConfigGroup uiConfig(config, "User Interface");
    int wc = mainWindows().size();
    uiConfig.writeEntry("Main Windows Count", wc);
    for (int w = 0; w < wc; ++w)
    {
        KConfigGroup mainWindowConfig(&uiConfig,
                                      QStringLiteral("Main Window %1").arg(w));

        for (Sublime::Area* defaultArea : defaultAreas()) {
            // FIXME: using object name seems ugly.
            QString type = defaultArea->objectName();
            Sublime::Area* area = this->area(w, type);
            KConfigGroup areaConfig(&mainWindowConfig, QLatin1String("Area ") + type);

            areaConfig.deleteGroup();
            areaConfig.writeEntry("id", type);
            saveArea(area, areaConfig);
            areaConfig.sync();
        }
    }
    uiConfig.sync();
}

void UiController::loadAllAreas(const KSharedConfigPtr& config)
{
    Q_D(UiController);

    KConfigGroup uiConfig(config, "User Interface");
    int wc = uiConfig.readEntry("Main Windows Count", 1);

    /* It is expected the main windows are restored before
       restoring areas.  */
    if (wc > mainWindows().size())
        wc = mainWindows().size();

    /* Offer all tool views to the default areas.  */
    for (Sublime::Area* area : defaultAreas()) {
        QHash<IToolViewFactory*, Sublime::ToolDocument*>::const_iterator i, e;
        for (i = d->factoryDocuments.constBegin(),
                 e = d->factoryDocuments.constEnd(); i != e; ++i)
        {
            addToolViewIfWanted(i.key(), i.value(), area);
        }
    }

    /* Restore per-windows areas.  */
    for (int w = 0; w < wc; ++w)
    {
        KConfigGroup mainWindowConfig(&uiConfig,
                                      QStringLiteral("Main Window %1").arg(w));

        Sublime::MainWindow *mw = mainWindows()[w];

        /* We loop over default areas.  This means that if
           the config file has an area of some type that is not
           in default set, we'd just ignore it.  I think it's fine --
           the model were a given mainwindow can has it's own
           area types not represented in the default set is way
           too complex.  */
        for (Sublime::Area* defaultArea : defaultAreas()) {
            QString type = defaultArea->objectName();
            Sublime::Area* area = this->area(w, type);

            KConfigGroup areaConfig(&mainWindowConfig, QLatin1String("Area ") + type);

            qCDebug(SHELL) << "Trying to restore area " << type;

            /* This is just an easy check that a group exists, to
               avoid "restoring" area from empty config group, wiping
               away programmatically installed defaults.  */
            if (areaConfig.readEntry("id", "") == type)
            {
                qCDebug(SHELL) << "Restoring area " << type;
                loadArea(area, areaConfig);
            }

            // At this point we know which tool views the area wants.
            // Tender all tool views we have.
            QHash<IToolViewFactory*, Sublime::ToolDocument*>::const_iterator i, e;
            for (i = d->factoryDocuments.constBegin(),
                     e = d->factoryDocuments.constEnd(); i != e; ++i)
            {
                addToolViewIfWanted(i.key(), i.value(), area);
            }
        }

        // Force reload of the changes.
        showAreaInternal(mw->area(), mw);

        mw->enableAreaSettingsSave();
    }

    d->areasRestored = true;
}

void UiController::addToolViewToDockArea(IToolViewFactory* factory, Qt::DockWidgetArea area)
{
    Q_D(UiController);

    addToolViewToArea(factory, d->factoryDocuments.value(factory), activeArea(), Sublime::dockAreaToPosition(area));
}

bool UiController::toolViewPresent(Sublime::ToolDocument* doc, Sublime::Area* area)
{
    for (Sublime::View *view : doc->views()) {
        if( area->toolViews().contains( view ) )
            return true;
    }
    return false;
}

void UiController::addToolViewIfWanted(IToolViewFactory* factory,
                           Sublime::ToolDocument* doc,
                           Sublime::Area* area)
{
    if (area->wantToolView(factory->id()))
    {
        addToolViewToArea(factory, doc, area);
    }
}

Sublime::View* UiController::addToolViewToArea(IToolViewFactory* factory,
                                     Sublime::ToolDocument* doc,
                                     Sublime::Area* area, Sublime::Position p)
{
    Sublime::View* view = doc->createView();
    area->addToolView(
        view,
        p == Sublime::AllPositions ? Sublime::dockAreaToPosition(factory->defaultPosition()) : p);

    connect(view, &Sublime::View::raise,
            this, QOverload<Sublime::View*>::of(&UiController::raiseToolView));

    factory->viewCreated(view);
    return view;
}

void UiController::registerStatus(QObject* status)
{
    Sublime::MainWindow* w = activeSublimeWindow();
    if (!w) return;
    auto* mw = qobject_cast<KDevelop::MainWindow*>(w);
    if (!mw) return;
    mw->registerStatus(status);
}

void UiController::showErrorMessage(const QString& message, int timeout)
{
    Sublime::MainWindow* w = activeSublimeWindow();
    if (!w) return;
    auto* mw = qobject_cast<KDevelop::MainWindow*>(w);
    if (!mw) return;
    QMetaObject::invokeMethod(mw, "showErrorMessage", Q_ARG(QString, message), Q_ARG(int, timeout));
}

const QHash< IToolViewFactory*, Sublime::ToolDocument* >& UiController::factoryDocuments() const
{
    Q_D(const UiController);

    return d->factoryDocuments;
}

QWidget* UiController::activeToolViewActionListener() const
{
    Q_D(const UiController);

    return d->activeActionListener;
}

QList<Sublime::Area*> UiController::allAreas() const
{
    return Sublime::Controller::allAreas();
}

}

#include "uicontroller.moc"
#include "moc_uicontroller.cpp"
