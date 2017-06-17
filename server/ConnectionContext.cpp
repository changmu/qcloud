#include "ConnectionContext.h"


ConnectionContext::ConnectionContext(State state)
  : state_(state)
{
}

ConnectionContext::State ConnectionContext::getState() const
{
    return state_;
}

void ConnectionContext::setState(const State &state)
{
    state_ = state;
}

EntryPtr ConnectionContext::getEntryPtr() const
{
    return wkEntryPtr_.lock();
}

void ConnectionContext::setWkEntryPtr(const EntryPtr &entryPtr)
{
    wkEntryPtr_ = entryPtr;
}

