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

#ifndef SV_EVENT_COMMANDS_H
#define SV_EVENT_COMMANDS_H

#include "base/Event.h"
#include "base/Command.h"
#include "base/ById.h"

/**
 * Interface for classes that can be modified through these commands
 */
class EventEditable
{
public:
    virtual void add(Event e) = 0;
    virtual void remove(Event e) = 0;
};

template <typename Base>
class WithEditable
{
    typedef typename Base::Id Id;
    Id m_id;
    
protected:
    WithEditable(Id id) : m_id(id) { }

    std::shared_ptr<EventEditable> getEditable() {
        auto base = StaticById<Base, Id>::get(m_id);
        if (!base) return {}; // acceptable - item has expired
        auto editable = std::dynamic_pointer_cast<EventEditable>(base);
        if (!editable) {
            SVCERR << "WARNING: Id passed to EventEditable command is not that of an EventEditable" << endl;
        }
        return editable;
    }
};

/**
 * Command to add an event to an editable containing events, with
 * undo.  The template parameter must be a type that can be
 * dynamic_cast to EventEditable and that has a ById store.
 */
template <typename Base>
class AddEventCommand : public Command,
                        public WithEditable<Base>
{
public:
    AddEventCommand(typename Base::Id editable, const Event &e, QString name) :
        WithEditable<Base>(editable), m_event(e), m_name(name) { }

    QString getName() const override { return m_name; }
    Event getEvent() const { return m_event; }

    void execute() override {
        auto editable = getEditable();
        if (editable) editable->add(m_event);
    }
    void unexecute() override {
        auto editable = getEditable();
        if (editable) editable->remove(m_event);
    }

private:
    Event m_event;
    QString m_name;
    using WithEditable<Base>::getEditable;
};

/**
 * Command to remove an event from an editable containing events, with
 * undo.  The template parameter must be a type that implements
 * EventBase and that has a ById store.
 */
template <typename Base>
class RemoveEventCommand : public Command,
                           public WithEditable<Base>
{
public:
    RemoveEventCommand(typename Base::Id editable, const Event &e, QString name) :
        WithEditable<Base>(editable), m_event(e), m_name(name) { }

    QString getName() const override { return m_name; }
    Event getEvent() const { return m_event; }

    void execute() override {
        auto editable = getEditable();
        if (editable) editable->remove(m_event);
    }
    void unexecute() override {
        auto editable = getEditable();
        if (editable) editable->add(m_event);
    }

private:
    Event m_event;
    QString m_name;
    using WithEditable<Base>::getEditable;
};

/**
 * Command to add or remove a series of events to or from an editable,
 * with undo. Creates and immediately executes a sub-command for each
 * add/remove requested. Consecutive add/remove pairs for the same
 * point are collapsed.  The template parameter must be a type that
 * implements EventBase and that has a ById store.
 */
template <typename Base>
class ChangeEventsCommand : public MacroCommand
{
    typedef typename Base::Id Id;
    
public:
    ChangeEventsCommand(Id editable, QString name) :
        MacroCommand(name), m_editable(editable) { }

    void add(Event e) {
        addCommand(new AddEventCommand<Base>(m_editable, e, getName()),
                   true);
    }
    void remove(Event e) {
        addCommand(new RemoveEventCommand<Base>(m_editable, e, getName()),
                   true);
    }

    /**
     * Stack an arbitrary other command in the same sequence.
     */
    void addCommand(Command *command) override { addCommand(command, true); }

    /**
     * If any points have been added or deleted, return this
     * command (so the caller can add it to the command history).
     * Otherwise delete the command and return NULL.
     */
    ChangeEventsCommand *finish() {
        if (!m_commands.empty()) {
            return this;
        } else {
            delete this;
            return nullptr;
        }
    }

protected:
    virtual void addCommand(Command *command, bool executeFirst) {
        
        if (executeFirst) command->execute();

        if (m_commands.empty()) {
            MacroCommand::addCommand(command);
            return;
        }
        
        RemoveEventCommand<Base> *r =
            dynamic_cast<RemoveEventCommand<Base> *>(command);
        AddEventCommand<Base> *a =
            dynamic_cast<AddEventCommand<Base> *>(*m_commands.rbegin());
        if (r && a) {
            if (a->getEvent() == r->getEvent()) {
                deleteCommand(a);
                return;
            }
        }
        
        MacroCommand::addCommand(command);
    }

    Id m_editable;
};

#endif
