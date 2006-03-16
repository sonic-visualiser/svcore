/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

/*
   This is a modified version of a source file from the Rosegarden
   MIDI and audio sequencer and notation editor, copyright 2000-2006
   Chris Cannam, distributed under the GNU General Public License.

   This file contains traces of the KCommandHistory class from the KDE
   project, copyright 2000 Werner Trobin and David Faure and
   distributed under the GNU Lesser General Public License.
*/

#include "CommandHistory.h"

#include "Command.h"

#include <QRegExp>
#include <QMenu>
#include <QToolBar>
#include <QString>

#include <iostream>

CommandHistory *CommandHistory::m_instance = 0;

CommandHistory::CommandHistory() :
    m_undoLimit(50),
    m_redoLimit(50),
    m_menuLimit(15),
    m_savedAt(0),
    m_currentMacro(0),
    m_executeMacro(false)
{
    m_undoAction = new QAction(QIcon(":/icons/undo.png"), tr("&Undo"), this);
    m_undoAction->setShortcut(tr("Ctrl+Z"));
    connect(m_undoAction, SIGNAL(triggered()), this, SLOT(undo()));
    
    m_undoMenuAction = new QAction(QIcon(":/icons/undo.png"), tr("&Undo"), this);
    connect(m_undoMenuAction, SIGNAL(triggered()), this, SLOT(undo()));
    
    m_undoMenu = new QMenu(tr("&Undo"));
    m_undoMenuAction->setMenu(m_undoMenu);
    connect(m_undoMenu, SIGNAL(triggered(QAction *)),
	    this, SLOT(undoActivated(QAction*)));

    m_redoAction = new QAction(QIcon(":/icons/redo.png"), tr("Re&do"), this);
    m_redoAction->setShortcut(tr("Ctrl+Shift+Z"));
    connect(m_redoAction, SIGNAL(triggered()), this, SLOT(redo()));
    
    m_redoMenuAction = new QAction(QIcon(":/icons/redo.png"), tr("Re&do"), this);
    connect(m_redoMenuAction, SIGNAL(triggered()), this, SLOT(redo()));

    m_redoMenu = new QMenu(tr("Re&do"));
    m_redoMenuAction->setMenu(m_redoMenu);
    connect(m_redoMenu, SIGNAL(triggered(QAction *)),
	    this, SLOT(redoActivated(QAction*)));
}

CommandHistory::~CommandHistory()
{
    m_savedAt = -1;
    clearStack(m_undoStack);
    clearStack(m_redoStack);

    delete m_undoMenu;
    delete m_redoMenu;
}

CommandHistory *
CommandHistory::getInstance()
{
    if (!m_instance) m_instance = new CommandHistory();
    return m_instance;
}

void
CommandHistory::clear()
{
    m_savedAt = -1;
    clearStack(m_undoStack);
    clearStack(m_redoStack);
    updateActions();
}

void
CommandHistory::registerMenu(QMenu *menu)
{
    menu->addAction(m_undoAction);
    menu->addAction(m_redoAction);
}

void
CommandHistory::registerToolbar(QToolBar *toolbar)
{
    toolbar->addAction(m_undoMenuAction);
    toolbar->addAction(m_redoMenuAction);
}

void
CommandHistory::addCommand(Command *command, bool execute)
{
    if (!command) return;

    if (m_currentMacro) {
	addToMacro(command);
	return;
    }

    std::cerr << "MVCH::addCommand: " << command->getName().toLocal8Bit().data() << " at " << command << std::endl;

    // We can't redo after adding a command
    clearStack(m_redoStack);

    // can we reach savedAt?
    if ((int)m_undoStack.size() < m_savedAt) m_savedAt = -1; // nope

    m_undoStack.push(command);
    clipCommands();
    
    if (execute) {
	command->execute();
    }

    // Emit even if we aren't executing the command, because
    // someone must have executed it for this to make any sense
    emit commandExecuted();
    emit commandExecuted(command);

    updateActions();
}

void
CommandHistory::addToMacro(Command *command)
{
    std::cerr << "MVCH::addToMacro: " << command->getName().toLocal8Bit().data() << std::endl;

    if (m_executeMacro) command->execute();
    m_currentMacro->addCommand(command);
}

void
CommandHistory::startCompoundOperation(QString name, bool execute)
{
    if (m_currentMacro) {
	std::cerr << "MVCH::startCompoundOperation: ERROR: compound operation already in progress!" << std::endl;
	std::cerr << "(name is " << m_currentMacro->getName().toLocal8Bit().data() << ")" << std::endl;
    }
    
    m_currentMacro = new MacroCommand(name);
    m_executeMacro = execute;
}

void
CommandHistory::endCompoundOperation()
{
    if (!m_currentMacro) {
	std::cerr << "MVCH::endCompoundOperation: ERROR: no compound operation in progress!" << std::endl;
    }

    Command *toAdd = m_currentMacro;
    m_currentMacro = 0;

    // We don't execute the macro command here, because we have been
    // executing the individual commands as we went along if
    // m_executeMacro was true.
    addCommand(toAdd, false);
}    

void
CommandHistory::addExecutedCommand(Command *command)
{
    addCommand(command, false);
}

void
CommandHistory::addCommandAndExecute(Command *command)
{
    addCommand(command, true);
}

void
CommandHistory::undo()
{
    if (m_undoStack.empty()) return;

    Command *command = m_undoStack.top();
    command->unexecute();
    emit commandExecuted();
    emit commandUnexecuted(command);

    m_redoStack.push(command);
    m_undoStack.pop();

    clipCommands();
    updateActions();

    if ((int)m_undoStack.size() == m_savedAt) emit documentRestored();
}

void
CommandHistory::redo()
{
    if (m_redoStack.empty()) return;

    Command *command = m_redoStack.top();
    command->execute();
    emit commandExecuted();
    emit commandExecuted(command);

    m_undoStack.push(command);
    m_redoStack.pop();
    // no need to clip

    updateActions();

    if ((int)m_undoStack.size() == m_savedAt) emit documentRestored();
}

void
CommandHistory::setUndoLimit(int limit)
{
    if (limit > 0 && limit != m_undoLimit) {
        m_undoLimit = limit;
        clipCommands();
    }
}

void
CommandHistory::setRedoLimit(int limit)
{
    if (limit > 0 && limit != m_redoLimit) {
        m_redoLimit = limit;
        clipCommands();
    }
}

void
CommandHistory::setMenuLimit(int limit)
{
    m_menuLimit = limit;
    updateActions();
}

void
CommandHistory::documentSaved()
{
    m_savedAt = m_undoStack.size();
}

void
CommandHistory::clipCommands()
{
    if ((int)m_undoStack.size() > m_undoLimit) {
	m_savedAt -= (m_undoStack.size() - m_undoLimit);
    }

    clipStack(m_undoStack, m_undoLimit);
    clipStack(m_redoStack, m_redoLimit);
}

void
CommandHistory::clipStack(CommandStack &stack, int limit)
{
    int i;

    if ((int)stack.size() > limit) {

	CommandStack tempStack;

	for (i = 0; i < limit; ++i) {
	    Command *command = stack.top();
	    std::cerr << "MVCH::clipStack: Saving recent command: " << command->getName().toLocal8Bit().data() << " at " << command << std::endl;
	    tempStack.push(stack.top());
	    stack.pop();
	}

	clearStack(stack);

	for (i = 0; i < m_undoLimit; ++i) {
	    stack.push(tempStack.top());
	    tempStack.pop();
	}
    }
}

void
CommandHistory::clearStack(CommandStack &stack)
{
    while (!stack.empty()) {
	Command *command = stack.top();
	// Not safe to call getName() on a command about to be deleted
	std::cerr << "MVCH::clearStack: About to delete command " << command << std::endl;
	delete command;
	stack.pop();
    }
}

void
CommandHistory::undoActivated(QAction *action)
{
    int pos = m_actionCounts[action];
    for (int i = 0; i <= pos; ++i) {
	undo();
    }
}

void
CommandHistory::redoActivated(QAction *action)
{
    int pos = m_actionCounts[action];
    for (int i = 0; i <= pos; ++i) {
	redo();
    }
}

void
CommandHistory::updateActions()
{
    m_actionCounts.clear();

    for (int undo = 0; undo <= 1; ++undo) {

	QAction *action(undo ? m_undoAction : m_redoAction);
	QAction *menuAction(undo ? m_undoMenuAction : m_redoMenuAction);
	QMenu *menu(undo ? m_undoMenu : m_redoMenu);
	CommandStack &stack(undo ? m_undoStack : m_redoStack);

	if (stack.empty()) {

	    QString text(undo ? tr("Nothing to undo") : tr("Nothing to redo"));

	    action->setEnabled(false);
	    action->setText(text);

	    menuAction->setEnabled(false);
	    menuAction->setText(text);

	} else {

	    action->setEnabled(true);
	    menuAction->setEnabled(true);

	    QString commandName = stack.top()->getName();
	    commandName.replace(QRegExp("&"), "");

	    QString text = (undo ? tr("&Undo %1") : tr("Re&do %1"))
		.arg(commandName);

	    action->setText(text);
	    menuAction->setText(text);
	}

	menu->clear();

	CommandStack tempStack;
	int j = 0;

	while (j < m_menuLimit && !stack.empty()) {

	    Command *command = stack.top();
	    tempStack.push(command);
	    stack.pop();

	    QString commandName = command->getName();
	    commandName.replace(QRegExp("&"), "");

	    QString text;
	    if (undo) text = tr("&Undo %1").arg(commandName);
	    else      text = tr("Re&do %1").arg(commandName);
	    
	    QAction *action = menu->addAction(text);
	    m_actionCounts[action] = j++;
	}

	while (!tempStack.empty()) {
	    stack.push(tempStack.top());
	    tempStack.pop();
	}
    }
}

