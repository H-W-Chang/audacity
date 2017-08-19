//
//  CommandFunctors.h
//  Audacity
//
//  Created by Paul Licameli on 4/22/16.
//
//

#ifndef __AUDACITY_COMMAND_FUNCTORS__
#define __AUDACITY_COMMAND_FUNCTORS__

#include <wx/string.h>
#include <wx/event.h>
#include "../MemoryX.h"

class AudacityProject;
class wxEvtHandler;

// Base class for objects, to whose member functions, the CommandManager will
// dispatch.
//
// It, or a subclass of it, must be the first base class of the object, and the
// first base class of that base class, etc., for the same reason that
// wxEvtHandler must be first (that is, the downcast from a pointer to the base
// to a pointer to the object, must be a vacuous operation).
//
// In fact, then, we just make it an alias of wxEvtHandler, in case you really
// need to inherit from wxEvtHandler for other reasons, and otherwise you
// couldn't satisfy the requirement for both base classes at once.
using CommandHandlerObject = wxEvtHandler;

using CommandHandlerFinder = CommandHandlerObject &(*)(AudacityProject&);

class wxEvent;

using CommandParameter = wxString;

struct CommandContext {
   CommandContext(
      AudacityProject &p
      , const wxEvent *e = nullptr
      , int ii = 0
      , const CommandParameter &param = {}
   )
      : project{ p }
      , pEvt{ e }
      , index{ ii }
      , parameter{ param }
   {}

   AudacityProject &project;
   const wxEvent *pEvt;
   int index;
   CommandParameter parameter;
};

class wxEvent;
typedef wxString PluginID;

class AUDACITY_DLL_API CommandFunctor /* not final */
{
public:
   CommandFunctor(){};
   virtual ~CommandFunctor(){};
   virtual void operator()(const CommandContext &context) = 0;
};

using CommandFunctorPointer = std::shared_ptr <CommandFunctor>;


// Define functor subclasses that dispatch to the correct call sequence on
// member functions of AudacityProject (or other class!)

template<typename OBJ>
using audCommandFunction = void (OBJ::*)();

template<typename OBJ>
class VoidFunctor final : public CommandFunctor
{
public:
   explicit VoidFunctor(OBJ *This, audCommandFunction<OBJ> pfn)
   : mThis{ This }, mCommandFunction{ pfn } {}
   void operator () (const CommandContext &context) override
   { (mThis->*mCommandFunction) (); }
private:
   OBJ *const mThis;
   const audCommandFunction<OBJ> mCommandFunction;
};

template<typename OBJ>
using audCommandKeyFunction = void (OBJ::*)(const wxEvent *);

template<typename OBJ>
class KeyFunctor final : public CommandFunctor
{
public:
   explicit KeyFunctor(OBJ *This, audCommandKeyFunction<OBJ> pfn)
   : mThis{ This }, mCommandKeyFunction{ pfn } {}
   void operator () (const CommandContext &context) override
   { (mThis->*mCommandKeyFunction) (context.pEvt); }
private:
   OBJ *const mThis;
   const audCommandKeyFunction<OBJ> mCommandKeyFunction;
};

template<typename OBJ>
using audCommandListFunction = void (OBJ::*)(int);

template<typename OBJ>
class ListFunctor final : public CommandFunctor
{
public:
   explicit ListFunctor(OBJ *This, audCommandListFunction<OBJ> pfn)
   : mThis{ This }, mCommandListFunction{ pfn } {}
   void operator () (const CommandContext &context) override
   { (mThis->*mCommandListFunction)(context.index); }
private:
   OBJ *const mThis;
   const audCommandListFunction<OBJ> mCommandListFunction;
};

template<typename OBJ>
using audCommandPluginFunction = void (OBJ::*)(const PluginID &, int);

template<typename OBJ>
class PluginFunctor final : public CommandFunctor
{
public:
   explicit PluginFunctor(OBJ *This, audCommandPluginFunction<OBJ> pfn)
   : mThis{ This }, mCommandPluginFunction{ pfn } {}
   void operator () (const CommandContext &context) override
   { (mThis->*mCommandPluginFunction)
      (context.parameter,
       0 // AudacityProject::OnEffectFlags::kNone
      ); }
private:
   OBJ *const mThis;
   const audCommandPluginFunction<OBJ> mCommandPluginFunction;
};

// Now define an overloaded factory function
template<typename OBJ>
inline CommandFunctorPointer MakeFunctor(OBJ *This,
                                         audCommandFunction<OBJ> pfn)
{ return CommandFunctorPointer{ safenew VoidFunctor<OBJ>{ This, pfn } }; }

template<typename OBJ>
inline CommandFunctorPointer MakeFunctor(OBJ *This,
                                         audCommandKeyFunction<OBJ> pfn)
{ return CommandFunctorPointer{ safenew KeyFunctor<OBJ>{ This, pfn } }; }

template<typename OBJ>
inline CommandFunctorPointer MakeFunctor(OBJ *This,
                                         audCommandListFunction<OBJ> pfn)
{ return CommandFunctorPointer{ safenew ListFunctor<OBJ>{ This, pfn } }; }

template<typename OBJ>
inline CommandFunctorPointer MakeFunctor(OBJ *This,
                                         audCommandPluginFunction<OBJ> pfn)
{ return CommandFunctorPointer{ safenew PluginFunctor<OBJ>{ This, pfn } }; }

// Now define the macro abbreviations that call the factory
#define FNT(OBJ, This, X) (MakeFunctor<OBJ>(This, X ))

#endif
