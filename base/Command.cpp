/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#include "Command.h"

MacroCommand::MacroCommand(QString name) :
    m_name(name)
{
}

MacroCommand::~MacroCommand()
{
    for (size_t i = 0; i < m_commands.size(); ++i) {
	delete m_commands[i];
    }
}

void
MacroCommand::addCommand(Command *command)
{
    m_commands.push_back(command);
}

void
MacroCommand::deleteCommand(Command *command)
{
    for (std::vector<Command *>::iterator i = m_commands.begin();
	 i != m_commands.end(); ++i) {

	if (*i == command) {
	    m_commands.erase(i);
	    delete command;
	    return;
	}
    }
}

void
MacroCommand::execute()
{
    for (size_t i = 0; i < m_commands.size(); ++i) {
	m_commands[i]->execute();
    }
}

void
MacroCommand::unexecute()
{
    for (size_t i = 0; i < m_commands.size(); ++i) {
	m_commands[m_commands.size() - i - 1]->unexecute();
    }
}

