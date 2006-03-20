/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <QString>
#include <vector>

class Command
{
public:
    virtual ~Command() { }

    virtual void execute() = 0;
    virtual void unexecute() = 0;
    virtual QString getName() const = 0;
};

class MacroCommand : public Command
{
public:
    MacroCommand(QString name);
    virtual ~MacroCommand();

    virtual void addCommand(Command *command);
    virtual void deleteCommand(Command *command);
    virtual bool haveCommands() const;

    virtual void execute();
    virtual void unexecute();

    virtual QString getName() const;
    virtual void setName(QString name);

protected:
    QString m_name;
    std::vector<Command *> m_commands;
};

#endif

