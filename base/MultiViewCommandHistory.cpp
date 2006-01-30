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

#include "MultiViewCommandHistory.h"

#include "Command.h"

#include <QRegExp>
#include <QMenu>
#include <QToolBar>
#include <QString>

#include <iostream>

MultiViewCommandHistory::MultiViewCommandHistory() :
    m_undoLimit(50),
    m_redoLimit(50),
    m_savedAt(0)
{
    m_undoAction = new QAction(QIcon(":/icons/undo.png"), tr("&Undo"), this);
    m_undoAction->setShortcut(tr("Ctrl+Z"));
    connect(m_undoAction, SIGNAL(triggered()), this, SLOT(undo()));
    
    m_undoMenu = new QMenu(tr("&Undo"));
    m_undoAction->setMenu(m_undoMenu);
    connect(m_undoMenu, SIGNAL(triggered(QAction *)),
	    this, SLOT(undoActivated(QAction*)));

    m_redoAction = new QAction(QIcon(":/icons/redo.png"), tr("&Redo"), this);
    m_redoAction->setShortcut(tr("Ctrl+Shift+Z"));
    connect(m_redoAction, SIGNAL(triggered()), this, SLOT(redo()));

    m_redoMenu = new QMenu(tr("Re&do"));
    m_redoAction->setMenu(m_redoMenu);
    connect(m_redoMenu, SIGNAL(triggered(QAction *)),
	    this, SLOT(redoActivated(QAction*)));
}

MultiViewCommandHistory::~MultiViewCommandHistory()
{
    m_savedAt = -1;
    clearStack(m_undoStack);
    clearStack(m_redoStack);

    delete m_undoMenu;
    delete m_redoMenu;
}

void
MultiViewCommandHistory::clear()
{
    m_savedAt = -1;
    clearStack(m_undoStack);
    clearStack(m_redoStack);
    updateActions();
}

void
MultiViewCommandHistory::registerMenu(QMenu *menu)
{
    menu->addAction(m_undoAction);
    menu->addAction(m_redoAction);
}

void
MultiViewCommandHistory::registerToolbar(QToolBar *toolbar)
{
    toolbar->addAction(m_undoAction);
    toolbar->addAction(m_redoAction);
}

void
MultiViewCommandHistory::addCommand(Command *command, bool execute)
{
    if (!command) return;

    std::cerr << "MVCH::addCommand: " << command->name().toLocal8Bit().data() << std::endl;

    // We can't redo after adding a command
    clearStack(m_redoStack);

    // can we reach savedAt?
    if ((int)m_undoStack.size() < m_savedAt) m_savedAt = -1; // nope

    m_undoStack.push(command);
    clipCommands();
    
    if (execute) {
	command->execute();
        emit commandExecuted();
	emit commandExecuted(command);
    }

//    updateButtons();
    updateActions();
}

void
MultiViewCommandHistory::undo()
{
    if (m_undoStack.empty()) return;

    Command *command = m_undoStack.top();
    command->unexecute();
    emit commandExecuted();
    emit commandExecuted(command);

    m_redoStack.push(command);
    m_undoStack.pop();

    clipCommands();
    updateActions();

    if ((int)m_undoStack.size() == m_savedAt) emit documentRestored();
}

void
MultiViewCommandHistory::redo()
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
}

void
MultiViewCommandHistory::setUndoLimit(int limit)
{
    if (limit > 0 && limit != m_undoLimit) {
        m_undoLimit = limit;
        clipCommands();
    }
}

void
MultiViewCommandHistory::setRedoLimit(int limit)
{
    if (limit > 0 && limit != m_redoLimit) {
        m_redoLimit = limit;
        clipCommands();
    }
}

void
MultiViewCommandHistory::documentSaved()
{
    m_savedAt = m_undoStack.size();
}

void
MultiViewCommandHistory::clipCommands()
{
    if ((int)m_undoStack.size() > m_undoLimit) {
	m_savedAt -= (m_undoStack.size() - m_undoLimit);
    }

    clipStack(m_undoStack, m_undoLimit);
    clipStack(m_redoStack, m_redoLimit);
}

void
MultiViewCommandHistory::clipStack(CommandStack &stack, int limit)
{
    int i;

    if ((int)stack.size() > limit) {

	CommandStack tempStack;

	for (i = 0; i < limit; ++i) {
	    Command *command = stack.top();
	    std::cerr << "MVCH::clipStack: Saving recent command: " << command->name().toLocal8Bit().data() << " at " << command << std::endl;
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
MultiViewCommandHistory::clearStack(CommandStack &stack)
{
    while (!stack.empty()) {
	Command *command = stack.top();
	std::cerr << "MVCH::clearStack: About to delete command: " << command->name().toLocal8Bit().data() << " at " << command << std::endl;
	delete command;
	stack.pop();
    }
}

void
MultiViewCommandHistory::undoActivated(QAction *action)
{
    int pos = m_actionCounts[action];
    for (int i = 0; i <= pos; ++i) {
	undo();
    }
}

void
MultiViewCommandHistory::redoActivated(QAction *action)
{
    int pos = m_actionCounts[action];
    for (int i = 0; i <= pos; ++i) {
	redo();
    }
}

void
MultiViewCommandHistory::updateActions()
{
    if (m_undoStack.empty()) {
	m_undoAction->setEnabled(false);
	m_undoAction->setText(tr("Nothing to undo"));
    } else {
	m_undoAction->setEnabled(true);
	QString commandName = m_undoStack.top()->name();
	commandName.replace(QRegExp("&"), "");
	QString text = tr("&Undo %1").arg(commandName);
	m_undoAction->setText(text);
    }

    if (m_redoStack.empty()) {
	m_redoAction->setEnabled(false);
	m_redoAction->setText(tr("Nothing to redo"));
    } else {
	m_redoAction->setEnabled(true);
	QString commandName = m_redoStack.top()->name();
	commandName.replace(QRegExp("&"), "");
	QString text = tr("Re&do %1").arg(commandName);
	m_redoAction->setText(text);
    }

    m_actionCounts.clear();

    for (int undo = 0; undo <= 1; ++undo) {

	QMenu *menu(undo ? m_undoMenu : m_redoMenu);
	CommandStack &stack(undo ? m_undoStack : m_redoStack);

	menu->clear();

	CommandStack tempStack;
	int j = 0;

	while (j < 10 && !stack.empty()) {

	    Command *command = stack.top();
	    tempStack.push(command);
	    stack.pop();

	    QString commandName = command->name();
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

