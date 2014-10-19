#include <QFontComboBox>
#include <QFontDatabase>
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>

#include "editor.h"
#include "closuremethodcall.h"

#include <freetype/font.h>

#include <label.h>
#include <renderer.h>
#include <keymouseconditions.h>

#include <input/controls.h>

#include <pipeline/pipelinetask.h>
#include <pipeline/pipeline.h>

#include <db/load.h>
#include <db/save.h>
#include <db/dbdatabase.h>

#include <resource/tree.h>

#include "core/saddythread.h"
#include "core/synchronization.h"
#include "core/selection.h"

#include "core/borders/activeborder.h"
#include "core/borders/selectionborder.h"

#include "gui/eventfilter.h"
#include "gui/renderways.h"

#include "typeconverters/save.h"
#include "typeconverters/load.h"
#include "typeconverters/qcolortosadcolor.h"
#include "typeconverters/qcolortosadacolor.h"
#include "typeconverters/qstringtosadstring.h"
#include "typeconverters/qlistqlistqcolortosadvectorsadvectoracolor.h"
#include "typeconverters/qrectftosadrect2d.h"
#include "typeconverters/sadcolortoqcolor.h"
#include "typeconverters/sadacolortoqcolor.h"
#include "typeconverters/sadstringtoqstring.h"
#include "typeconverters/sadvectorsadvectoracolortoqlistqlistqcolor.h"
#include "typeconverters/sadrect2dtoqrectf.h"

#include "../blockedclosuremethodcall.h"

// =================== PUBLIC METHODS ===================

core::Editor::Editor()
{
	// Add message data
	m_qttarget = new core::QtTarget(this);
	sad::Renderer::ref()->log()->addTarget(m_qttarget);

	// Add small user log
	sad::log::FileTarget * t = new sad::log::FileTarget("{0}: [{1}] {3}{2}{4}", sad::log::MESSAGE);
	t->open("user.txt");
	sad::log::Log::ref()->addTarget(t);

	// Add large debug log
	t = new sad::log::FileTarget();
	t->open("full.txt");
	sad::log::Log::ref()->addTarget(t);

    m_args = NULL;
    m_renderthread = new core::SaddyThread(this);
    m_qtapp = NULL;
	m_history = new history::History();

	m_machine = new sad::hfsm::Machine();
	// A states for editing objects
	m_machine->addState("idle", new sad::hfsm::State(), true);
	m_machine->addState("selected", new sad::hfsm::State(), true);
	m_machine->addState("selected/moving", new sad::hfsm::State(), true);
	m_machine->addState("selected/resizing", new sad::hfsm::State(), true);
	m_machine->addState("adding/label", new sad::hfsm::State(), true);
	m_machine->addState("adding/sprite", new sad::hfsm::State(), true);
	m_machine->addState("adding/sprite_diagonal", new sad::hfsm::State(), true);
	m_machine->addState("adding/sprite_diagonal/point_placed", new sad::hfsm::State(), true);
	m_machine->addState("adding/customobject", new sad::hfsm::State(), true);
	m_machine->addState("adding/customobject_diagonal", new sad::hfsm::State(), true);
	m_machine->addState("adding/customobject_diagonal/point_placed", new sad::hfsm::State(), true);
	// A states for editing ways
	m_machine->addState("ways/idle", new sad::hfsm::State(), true);
	m_machine->addState("ways/selected", new sad::hfsm::State(), true);
	m_machine->addState("ways/selected/moving", new sad::hfsm::State(), true);

	m_machine->enterState("idle");

	m_shared = new core::Shared();
	m_shared->setEditor(this);

	m_active_border = new core::borders::ActiveBorder(m_shared);
	m_selection_border = new core::borders::SelectionBorder(m_shared);
	
    m_synchronization = new core::Synchronization();

	sad::String idle = "idle";
	sad::Renderer::ref()->controls()->add(*sad::input::ET_KeyPress & sad::Esc & (m_machine * idle), sad::Renderer::ref(), &sad::Renderer::quit);
	sad::Renderer::ref()->controls()->add(*sad::input::ET_KeyPress & sad::Z & sad::HoldsControl, this, &core::Editor::undo);
	sad::Renderer::ref()->controls()->add(*sad::input::ET_KeyPress & sad::R & sad::HoldsControl, this, &core::Editor::redo);

	sad::Renderer::ref()->pipeline()->append(m_selection_border);
	sad::Renderer::ref()->pipeline()->append(m_active_border);
    m_renderways = new gui::RenderWays(this);
    sad::Renderer::ref()->pipeline()->append(m_renderways);


	m_selection = new core::Selection();
	m_selection->setEditor(this);

	// Fill conversion table with converters
	this->initConversionTable();
}
core::Editor::~Editor()
{	
    delete m_args;
	delete m_qtapp;
	delete m_renderthread;
    delete m_parsed_args;
	delete m_history;
	delete m_shared;
	delete m_machine;
    delete m_synchronization;
	delete m_selection;
}

void core::Editor::init(int argc,char ** argv)
{
	// Create and parse command line arguments
    m_args = new sad::cli::Args(argc, argv);
    m_parsed_args = new sad::cli::Parser();
    m_parsed_args->addSingleValuedOption("resources");
    m_parsed_args->addSingleValuedOption("width");
    m_parsed_args->addSingleValuedOption("height");
    m_parsed_args->addFlag("debug");
    m_parsed_args->parse(argc, const_cast<const char **>(argv));
	
	// This thread only runs qt event loop. SaddyThread runs only event loop of renderer of Saddy.
    m_synchronization->startSynchronizationWithSaddyThread();
	m_renderthread->start();
	// Wait for Saddy's window to show up
    m_synchronization->waitForSaddyThread();
	this->runQtEventLoop();
	m_renderthread->wait();
}

MainPanel* core::Editor::panel() const
{
	return m_mainwindow;
}

sad::hfsm::Machine* core::Editor::machine() const
{
	return m_machine;
}

core::Shared* core::Editor::shared() const
{
    return m_shared;
}

sad::cli::Parser * core::Editor::parsedArgs() const
{
    return m_parsed_args;
}

history::History* core::Editor::history() const
{
    return m_history;
}

QApplication* core::Editor::app() const
{
    return m_qtapp;
}

sad::Renderer* core::Editor::renderer() const
{
    return sad::Renderer::ref();
}

core::Synchronization* core::Editor::synchronization() const
{
    return m_synchronization;
}

core::borders::ActiveBorder* core::Editor::activeBorder() const
{
	return m_active_border;
}

core::borders::SelectionBorder* core::Editor::selectionBorder() const
{
	return m_selection_border;
}

gui::RenderWays* core::Editor::renderWays() const
{
    return m_renderways;
}

core::Selection* core::Editor::selection() const
{
	return m_selection;
}

void core::Editor::quit()
{
    sad::Renderer::ref()->quit();
}

void core::Editor::emitClosure(sad::ClosureBasic * closure)
{
    emit closureArrived(closure);
}

void core::Editor::cleanupBeforeAdding()
{
	if (this->machine()->isInState("adding"))
	{
		sad::Renderer::ref()->lockRendering();

		sad::SceneNode* node = this->shared()->activeObject();
		this->shared()->setActiveObject(NULL);
		node->scene()->remove(node);

		sad::Renderer::ref()->unlockRendering();

		this->panel()->clearCustomObjectPropertiesTable();
	}
}

bool core::Editor::isNodeSelected(sad::SceneNode* node) const
{
    return node  == m_shared->selectedObject() && m_machine->isInState("selected");
}

void core::Editor::enteredIdleState()
{
	m_mainwindow->highlightIdleState();
	this->emitClosure( bind(m_mainwindow, &MainPanel::clearCustomObjectPropertiesTable));
}

static const size_t CoreEditorEditingStatesCount = 4; 

static sad::String CoreEditorEditingStates[CoreEditorEditingStatesCount] = {
	sad::String("adding"),
	sad::String("selected/moving"),
	sad::String("selected/resizing"),
	sad::String("ways/selected/moving")
};

bool core::Editor::isInEditingState() const
{
	bool result = false;
	for (size_t i = 0; i < CoreEditorEditingStatesCount; ++i)
	{
		result = result || m_machine->isInState(CoreEditorEditingStates[i]);
	}
	return result;
}

void core::Editor::cleanDatabase()
{
	m_machine->enterState("idle");
	m_shared->setSelectedObject(NULL);
	m_shared->setSelectedWay(NULL);
	m_shared->setSelectedDialogue(NULL);
	m_shared->setActiveObject(NULL);
	m_history->clear();
	m_mainwindow->clearDatabaseProperties();
	sad::Renderer::ref()->clear();
	m_mainwindow->UI()->lstSceneObjects->clear();
	m_mainwindow->UI()->lstScenes->clear();
	m_mainwindow->UI()->lstWays->clear();
	m_mainwindow->UI()->lstWayPoints->clear();
	m_mainwindow->UI()->lstDialogues->clear();
	m_mainwindow->UI()->lstPhrases->clear();
	m_mainwindow->clearDatabaseProperties();
	sad::Renderer::ref()->removeDatabase("");
}

void core::Editor::reportResourceLoadingErrors(
        sad::Vector<sad::resource::Error *> & errors,
        const sad::String& name
)
{
    sad::String errorlist = sad::resource::format(errors);
    sad::String resultmessage = "There was errors while loading ";
    resultmessage += name;
    resultmessage += ":\n";
    resultmessage += errorlist;
    sad::util::free(errors);
    errors.clear();
    SL_FATAL(resultmessage);
}

bool core::Editor::isInObjectEditingState() const
{
	return m_mainwindow->UI()->tabTypes->currentIndex() == 0;
}

bool core::Editor::isInWaysEditingState() const
{
	return m_mainwindow->UI()->tabTypes->currentIndex() == 1;
}

void core::Editor::tryEnterObjectEditingState()
{
	if (this->isInEditingState())
	{
		return;
	}
	this->m_machine->enterState("idle");
	m_shared->setSelectedObject(NULL);
	m_shared->setActiveObject(NULL);
	m_shared->setSelectedWay(NULL);
	invoke_blocked(m_mainwindow->UI()->tabTypes, &QTabWidget::setCurrentIndex, 0);
}

void core::Editor::tryEnterWayEditingState()
{
	if (this->isInEditingState())
	{
		return;
	}
	this->m_machine->enterState("idle");
	this->m_machine->enterState("ways/idle");
	m_shared->setSelectedObject(NULL);
	m_shared->setActiveObject(NULL);
	m_shared->setSelectedWay(NULL);	
	invoke_blocked(m_mainwindow->UI()->tabTypes, &QTabWidget::setCurrentIndex, 1);
}

// =================== PUBLIC SLOTS METHODS ===================

void core::Editor::start()
{
	SL_SCOPE("core::Editor::start()");
	bool mustquit = false;

	// Try load icons resources
	sad::Renderer::ref()->addTree("icons", new sad::resource::Tree());
	sad::Renderer::ref()->tree("icons")->factory()->registerResource<sad::freetype::Font>();
	sad::Vector<sad::resource::Error * > errors;
	errors = sad::Renderer::ref()->tree("icons")->loadFromFile(ICONS_PATH);
	if (errors.size())
	{
		mustquit = true;
		this->reportResourceLoadingErrors(errors, ICONS_PATH);
	}

	// Try load specified resources, if need to
	sad::Renderer::ref()->tree("")->factory()->registerResource<sad::freetype::Font>();
    sad::Renderer::ref()->tree("")->setStoreLinks(true);
    sad::Maybe<sad::String> maybefilename = this->parsedArgs()->single("resources");
	if (maybefilename.exists() && this->parsedArgs()->specified("resources"))
	{
		errors = sad::Renderer::ref()->tree("")->loadFromFile(maybefilename.value());
		if (errors.size())
		{
			mustquit = true;
			this->reportResourceLoadingErrors(errors, maybefilename.value());
		}
        else
        {
            this->m_mainwindow->updateResourceViews();
        }
	}
	else
	{
		this->m_mainwindow->toggleEditingButtons(false);
	}
	
	bool database_loaded = false;
	if (this->m_mainwindow->isEditingEnabled() && this->parsedArgs()->defaultOption().exists() && !mustquit)
	{		
		// Try load database	
		sad::String value = this->parsedArgs()->defaultOption().value();
		sad::db::Database* tmp = new sad::db::Database();
		tmp->setRenderer(sad::Renderer::ref());
		tmp->setDefaultTreeName("");
		if (tmp->loadFromFile(value, sad::Renderer::ref()))
		{
			this->shared()->setFileName(value.data());
			sad::Renderer::ref()->addDatabase("", tmp);
			database_loaded = true;
		}
		else
		{
			delete tmp;
			mustquit = true;
			SL_FATAL("Failed to load database");
		}
	}

	// If no database loaded, init default database, add a palette to it.
	if (database_loaded == false)
	{
		// Create a default database
		sad::db::Database * db = new sad::db::Database();
		// Default database has empty name
		sad::Renderer::ref()->addDatabase("", db);
	}
	this->m_mainwindow->viewDatabase();

	if (mustquit)
	{
		 QTimer::singleShot(0, this->panel(), SLOT(close()));
	}
	else
	{
		m_renderways->enable();
	}
}

void core::Editor::undo()
{
	m_history->rollback(this);
}

void core::Editor::redo()
{
	m_history->commit(this);
}

// =================== PROTECTED METHODS ===================

void core::Editor::initConversionTable()
{
	sad::db::ConversionTable::ref()->add(
		"sad::Color", 
		"QColor", 
		new core::typeconverters::SadColorToQColor()
	);
	sad::db::ConversionTable::ref()->add(
		"sad::AColor", 
		"QColor", 
		new core::typeconverters::SadAColorToQColor()
	);
	sad::db::ConversionTable::ref()->add(
		"sad::String", 
		"QString", 
		new core::typeconverters::SadStringToQString()
	);
	sad::db::ConversionTable::ref()->add(
		"sad::Vector<sad::Vector<sad::AColor> >", 
		"QList<QList<QColor> >", 
		new core::typeconverters::SadVectorSadVectorToAColorToQListQListQColor()
	);
    sad::db::ConversionTable::ref()->add(
        "sad::Rect2D",
        "QRectF",
        new core::typeconverters::SadRect2DToQRectF()
    );
	sad::db::ConversionTable::ref()->add(
		"QColor", 
		"sad::Color", 		
		new core::typeconverters::QColorToSadColor()
	);
	sad::db::ConversionTable::ref()->add(
		"QColor", 		
		"sad::AColor", 
		new core::typeconverters::QColorToSadAColor()
	);
	sad::db::ConversionTable::ref()->add(
		"QString",
		"sad::String",  
		new core::typeconverters::QStringToSadString()
	);
	sad::db::ConversionTable::ref()->add(
		"QList<QList<QColor> >", 
		"sad::Vector<sad::Vector<sad::AColor> >", 
		new core::typeconverters::QListQListQColorToSadVectorSadVectorToAColor()
	);
    sad::db::ConversionTable::ref()->add(
        "sad::Rect2D",
        "QRectF",
        new core::typeconverters::SadRect2DToQRectF()
    );
}

void core::Editor::saddyQuitSlot()
{
    if (m_quit_reason == core::QR_NOTSET) {
        m_quit_reason = core::QR_SADDY;
        QTimer::singleShot(0,this,SLOT(onQuitActions()));
    }
}

// =================== PROTECTED SLOTS METHODS ===================

void core::Editor::runQtEventLoop()
{
    m_qtapp = new QApplication(m_args->count(), m_args->arguments());

	m_mainwindow = new MainPanel();
	m_mainwindow->setEditor(this);

	// Called this explicitly, because entered state before
	m_machine->state("idle")->addEnterHandler(this, &core::Editor::enteredIdleState);
	m_machine->state("selected")->addEnterHandler(m_mainwindow, &MainPanel::highlightSelectedState);
	m_machine->state("selected")->addEnterHandler(m_mainwindow, &MainPanel::updateUIForSelectedItem);
	m_machine->state("adding/label")->addEnterHandler(m_mainwindow, &MainPanel::highlightLabelAddingState);
	m_machine->state("ways/idle")->addEnterHandler(this, &core::Editor::enteredIdleState);
	m_machine->state("ways/selected")->addEnterHandler(m_mainwindow, &MainPanel::highlightSelectedState);

	m_mainwindow->highlightIdleState();

	sad::Renderer::ref()->controls()->add(*sad::input::ET_MouseMove, m_mainwindow, &MainPanel::updateMousePosition);

    gui::EventFilter* filter = new gui::EventFilter();
    filter->setPanel(m_mainwindow);
    QCoreApplication::instance()->installEventFilter(filter);

	QObject::connect(
		this->m_qtapp,
		SIGNAL(lastWindowClosed()),
		this,
		SLOT(qtQuitSlot()));
	this->m_mainwindow->show();

	QObject::connect(
		this, 
		SIGNAL(closureArrived(sad::ClosureBasic*)), 
		this, 
        SLOT(runClosure(sad::ClosureBasic*))
	);
	m_qttarget->enable();
	QTimer::singleShot(0, this, SLOT(start()));
	this->m_qtapp->exec();
	m_qttarget->disable();
}

void core::Editor::runSaddyEventLoop() 
{
	m_quit_reason = core::QR_NOTSET;
	sad::Renderer::ref()->run();
	// Quit reason can be set by main thread, when window is closed
	if (m_quit_reason == core::QR_NOTSET)
		this->saddyQuitSlot();
}

void core::Editor::qtQuitSlot()
{
	if (m_quit_reason == core::QR_NOTSET) {
		m_quit_reason = core::QR_QTWINDOW;
		this->onQuitActions();
	}
}
void core::Editor::onQuitActions()
{
	if (m_quit_reason == core::QR_SADDY) {
		this->m_mainwindow->close();
	}
	if (m_quit_reason == core::QR_QTWINDOW) {
		sad::Renderer::ref()->quit();
	}
}

void core::Editor::runClosure(sad::ClosureBasic * closure)
{
    closure->run();
    delete closure;
}
