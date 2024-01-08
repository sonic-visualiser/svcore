/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SV_COMMAND_H
#define SV_COMMAND_H

#include <QObject>
#include <QString>
#include <vector>
#include <functional>

#include "Debug.h"

namespace sv {

class Command
{
public:
    virtual ~Command() { }

    virtual void execute() = 0;
    virtual void unexecute() = 0;
    virtual QString getName() const = 0;
};

/** 
 * GenericCommand is a Command that can be constructed directly using
 * lambdas, without having to create a subclass. Best for commands
 * invoked only in a single place, and where little state is involved.
 */
class GenericCommand : public Command
{
public:
    GenericCommand(QString name,
                   std::function<void()> execute,
                   std::function<void()> unexecute) :
        m_name(name),
        m_execute(execute),
        m_unexecute(unexecute),
        m_onDelete([] {}) {
    }

    GenericCommand(QString name,
                   std::function<void()> execute,
                   std::function<void()> unexecute,
                   std::function<void()> onDelete) :
        m_name(name),
        m_execute(execute),
        m_unexecute(unexecute),
        m_onDelete(onDelete) {
    }
    
    virtual ~GenericCommand() {
        m_onDelete();
    }

    QString getName() const override {
        return m_name;
    }
    
    void execute() override {
        m_execute();
    }

    void unexecute() override {
        m_unexecute();
    }

private:
    QString m_name;
    std::function<void()> m_execute;
    std::function<void()> m_unexecute;
    std::function<void()> m_onDelete;
};    

class MacroCommand : public Command
{
public:
    MacroCommand(QString name);
    virtual ~MacroCommand();

    virtual void addCommand(Command *command);
    virtual void deleteCommand(Command *command);
    virtual bool haveCommands() const;

    void execute() override;
    void unexecute() override;

    QString getName() const override;
    virtual void setName(QString name);

protected:
    QString m_name;
    std::vector<Command *> m_commands;
};

/**
 * BundleCommand is a MacroCommand whose name includes a note of how
 * many commands it contains.  It is a QObject with Q_OBJECT macro so
 * that it can do plural-sensitive translations.
 */
class BundleCommand : public QObject, public MacroCommand
{
    Q_OBJECT

public:
    BundleCommand(QString name);
    virtual ~BundleCommand();

    QString getName() const override;
};

} // end namespace sv

#endif

