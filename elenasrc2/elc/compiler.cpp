//---------------------------------------------------------------------------
//		E L E N A   P r o j e c t:  ELENA Compiler
//
//		This file contains ELENA compiler class implementation.
//
//                                              (C)2005-2014, by Alexei Rakov
//---------------------------------------------------------------------------

#include "elena.h"
// --------------------------------------------------------------------------
#include "compiler.h"
#include "errors.h"
#include <errno.h>

using namespace _ELENA_;

// --- Hint constants ---
#define HINT_MASK             0xFFFF0000 
#define HINT_ROOT             0x80000000
#define HINT_ROOTEXPR         0x40000000
#define HINT_LOOP             0x20000000
#define HINT_TRY              0x10000000
#define HINT_CATCH            0x08000000
#define HINT_OUTEXPR          0x04000000
#define HINT_SUBBRANCH        0x02000000     // used for if-else statement to indicate that the exit label is not the last one
#define HINT_DIRECT_ORDER     0x01000000     // indictates that the parameter should be stored directly in reverse order
#define HINT_TYPEENFORCING    0x00800000     
#define HINT_GENERIC_MODE     0x00400000
#define HINT_OARG_UNBOXING    0x00200000     // used to indicate unboxing open argument list
#define HINT_GENERIC_METH     0x00100000     // generic methodcompileRetExpression
#define HINT_ASSIGN_MODE      0x00080000     // indicates possible assigning operation (e.g a := a + x)

// --- Method optimization masks ---
#define MTH_FRAME_USED        0x00000001

// --- Auxiliary routines ---

inline bool isCollection(DNode node)
{
   return (node == nsExpression && node.nextNode()==nsExpression);
}

inline bool isExpressionAction(DNode expr)
{
   return (expr == nsExpression && expr.nextNode() == nsNone);
}

inline ref_t importMessage(_Module* exporter, ref_t exportRef, _Module* importer)
{
   int verbId = 0;
   ref_t signRef = 0;
   int paramCount = 0;

   decodeMessage(exportRef, signRef, verbId, paramCount);

   // if it is generic message
   if (signRef == 0) {
      return exportRef;
   }

   // otherwise signature and custom verb should be imported
   if (signRef != 0) {
      const wchar16_t* subject = exporter->resolveSubject(signRef);

      signRef = importer->mapSubject(subject, false);
   }
   return encodeMessage(signRef, verbId, paramCount);
}

inline ref_t importSubject(_Module* exporter, ref_t exportRef, _Module* importer)
{
   // otherwise signature and custom verb should be imported
   if (exportRef != 0) {
      const wchar16_t* subject = exporter->resolveSubject(exportRef);

      exportRef = importer->mapSubject(subject, false);
   }
   return exportRef;
}

inline ref_t importReference(_Module* exporter, ref_t exportRef, _Module* importer)
{
   if (exportRef) {
      const wchar16_t* reference = exporter->resolveReference(exportRef);

      return importer->mapReference(reference);
   }
   else return 0;
}

inline void findUninqueName(_Module* module, ReferenceNs& name)
{
   size_t pos = getlength(name);
   int   index = 0;
   ref_t ref = 0;
   do {
      name[pos] = 0;
      name.appendHex(index++);

      ref = module->mapReference(name, true);
   } while (ref != 0);
}

// skip the hints and return the first hint node or none
inline DNode skipHints(DNode& node)
{
   DNode hints;
   if (node == nsHint)
      hints = node;

   while (node == nsHint)
      node = node.nextNode();

   return hints;
}

inline int countSymbol(DNode node, Symbol symbol)
{
   int counter = 0;
   while (node != nsNone) {
      if (node == symbol)
         counter++;

      node = node.nextNode();
   }
   return counter;
}

inline bool findSymbol(DNode node, Symbol symbol)
{
   while (node != nsNone) {
      if (node==symbol)
         return true;

      node = node.nextNode();
   }
   return false;
}

inline bool findSymbol(DNode node, Symbol symbol1, Symbol symbol2)
{
   while (node != nsNone) {
      if (node==symbol1 || node==symbol2)
         return true;

      node = node.nextNode();
   }
   return false;
}

inline DNode goToSymbol(DNode node, Symbol symbol)
{
   while (node != nsNone) {
      if (node==symbol)
         return node;

      node = node.nextNode();
   }
   return node;
}

inline bool IsArgumentList(ObjectInfo object)
{
   return object.kind == okParams;
}

inline bool isAssignOperation(DNode target, DNode operation)
{
   DNode lnode = operation.firstChild().firstChild();

   TerminalInfo tTerminal = target.Terminal();
   TerminalInfo lTerminal = lnode.Terminal();

   if (lTerminal.symbol == tsIdentifier && lTerminal.symbol == tsIdentifier && StringHelper::compare(lTerminal.value, tTerminal.value)) {
      if (operation.nextNode() == nsNone)
         return true;
   }

   return false;
}

inline ref_t getType(ObjectInfo object)
{
   switch(object.kind) {
      case okConstantClass:
      case okOuterField:
         return 0;
      default:
         return object.extraparam;
   }
}

inline bool IsExprOperator(int operator_id)
{
   switch(operator_id) {
      case ADD_MESSAGE_ID:
      case SUB_MESSAGE_ID:
      case MUL_MESSAGE_ID:
      case DIV_MESSAGE_ID:
      case AND_MESSAGE_ID:
      case OR_MESSAGE_ID:
      case XOR_MESSAGE_ID:
         return true;
      default:
         return false;
   }
}

inline bool IsCompOperator(int operator_id)
{
   switch(operator_id) {
      case EQUAL_MESSAGE_ID:
      case NOTEQUAL_MESSAGE_ID:
      case LESS_MESSAGE_ID:
      case NOTLESS_MESSAGE_ID:
      case GREATER_MESSAGE_ID:
      case NOTGREATER_MESSAGE_ID:
         return true;
      default:
         return false;
   }
}

inline bool IsReferOperator(int operator_id)
{
   return operator_id == REFER_MESSAGE_ID;
}

inline bool IsDoubleOperator(int operator_id)
{
   return operator_id == SET_REFER_MESSAGE_ID;
}

inline bool IsInvertedOperator(int& operator_id)
{
   if (operator_id == NOTEQUAL_MESSAGE_ID) {
      operator_id = EQUAL_MESSAGE_ID;

      return true;
   }
   else if (operator_id == NOTLESS_MESSAGE_ID) {
      operator_id = LESS_MESSAGE_ID;

      return true;
   }
   else if (operator_id == NOTGREATER_MESSAGE_ID) {
      operator_id = GREATER_MESSAGE_ID;

      return true;
   }
   else return false;
}

// --- Compiler::ModuleScope ---

Compiler::ModuleScope :: ModuleScope(Project* project, const tchar_t* sourcePath, Unresolveds* forwardsUnresolved)
   : symbolHints(okUnknown), extensions(NULL, freeobj)
{
   this->project = project;
   this->sourcePath = sourcePath;
   this->forwardsUnresolved = forwardsUnresolved;
   this->sourcePathRef = 0;

   intType = boolType = longType = realType = literalType = 0;

   warnOnUnresolved = project->BoolSetting(opWarnOnUnresolved);
   warnOnWeakUnresolved = project->BoolSetting(opWarnOnWeakUnresolved);
}

void Compiler::ModuleScope :: init(_Module* module, _Module* debugModule)
{
   this->module = module;
   this->debugModule = debugModule;

   // cache the frequently used references
   nilReference = mapConstantReference(NIL_CLASS);
   trueReference = mapConstantReference(TRUE_CLASS);
   falseReference = mapConstantReference(FALSE_CLASS);

   paramsType = mapSubject(PARAMS_SUBJECT);
   actionType = mapSubject(ACTION_SUBJECT);

   defaultNs.add(module->Name());

   // load system types
   loadTypes(project->loadModule(STANDARD_MODULE, true));
}

ref_t Compiler::ModuleScope :: mapSubject(const wchar16_t* name)
{
   ref_t sign_ref = module->mapSubject(name, false);

   return sign_ref;
}

ObjectInfo Compiler::ModuleScope :: mapObject(TerminalInfo identifier)
{
   if (identifier==tsReference) {
      return mapReferenceInfo(identifier, false);
   }
   else if (identifier==tsPrivate) {
      return defineObjectInfo(mapTerminal(identifier, true), true);
   }
   else if (identifier==tsIdentifier) {
      return defineObjectInfo(mapTerminal(identifier, true), true);
   }
   else return ObjectInfo();
}

ref_t Compiler::ModuleScope :: resolveIdentifier(const wchar16_t* identifier)
{
   List<const wchar16_t*>::Iterator it = defaultNs.start();
   while (!it.Eof()) {
      ReferenceNs name(*it, identifier);

      if (checkReference(name))
         return module->mapReference(name);

      it++;
   }
   return 0;
}

ref_t Compiler::ModuleScope :: mapType(const wchar16_t* terminal)
{
   ref_t subj_ref = module->mapSubject(terminal, true);
   if (subj_ref != 0) {
      if (typeHints.exist(subj_ref)) {
         return subj_ref;
      }
      else return synonymHints.get(subj_ref);
   }

   return 0;
}

ref_t Compiler::ModuleScope :: mapType(TerminalInfo terminal, bool& out)
{
   const wchar16_t* identifier = terminal;
   if (ConstantIdentifier::compare(identifier, "out'", 4)) {
      out = true;

      identifier += 4;
   }

   ref_t type_ref = mapType(identifier);

   return type_ref;
}

ref_t Compiler::ModuleScope :: mapTerminal(TerminalInfo terminal, bool existing)
{
   if (terminal == tsIdentifier) {
      ref_t reference = forwards.get(terminal);
      if (reference == 0) {
         if (!existing) {
            ReferenceNs name(module->Name(), terminal);

            return module->mapReference(name);
         }
         else return resolveIdentifier(terminal);
      }
      else return reference;
   }
   else if (terminal == tsPrivate) {
      ReferenceNs name(module->Name(), terminal);

      return mapReference(name, existing);
   }
   else return mapReference(terminal, existing);
}

bool Compiler::ModuleScope :: checkReference(const wchar16_t* referenceName)
{
   ref_t moduleRef = 0;
   _Module* module = project->resolveModule(referenceName, moduleRef, true);

   if (module == NULL || moduleRef == 0)
      return false;

   return module->mapReference(referenceName, true);
}

ObjectInfo Compiler::ModuleScope :: defineObjectInfo(ref_t reference, bool checkState)
{
   // if reference is zero the symbol is unknown
   if (reference == 0) {
      return ObjectInfo();
   }
   // check if symbol should be treated like constant one
   else if (symbolHints.exist(reference, okConstantSymbol)) {
      return ObjectInfo(okConstantSymbol, reference);
   }
   else if (symbolHints.exist(reference, okIntConstant)) {
      return ObjectInfo(okConstant, reference, getIntType());
   }
   else if (symbolHints.exist(reference, okLiteralConstant)) {
      return ObjectInfo(okConstant, reference, getLiteralType());
   }
   else if (checkState) {
      ClassInfo info;
      ref_t r = loadClassInfo(info, module->resolveReference(reference));
      if (r) {
         // if it is a role class or stateless symbol
         if (test(info.header.flags, elRole) || test(info.header.flags, elConstantSymbol)) {
            defineConstantSymbol(reference);

            return ObjectInfo(okConstantSymbol, reference);
         }
         // if it is a normal class
         // then the symbol is reference to the class class
         else if (test(info.header.flags, elStandartVMT) && info.classClassRef != 0) {
            return ObjectInfo(okConstantClass, reference, info.classClassRef);
         }
      }
   }
   // otherwise it is a normal one
   return ObjectInfo(okSymbol, reference);
}

ref_t Compiler::ModuleScope :: mapReference(const wchar16_t* referenceName, bool existing)
{
   ref_t reference = 0;
   if (!isWeakReference(referenceName)) {
      if (existing) {
         // check if the reference does exist
         ref_t moduleRef = 0;
         _Module* argModule = project->resolveModule(referenceName, moduleRef);

         if (argModule != NULL && moduleRef != 0)
            reference = module->mapReference(referenceName);
      }
      else reference = module->mapReference(referenceName, existing);
   }
   else reference = module->mapReference(referenceName, existing);

   return reference;
}

ObjectInfo Compiler::ModuleScope :: mapReferenceInfo(const wchar16_t* reference, bool existing)
{
   if (ConstantIdentifier::compare(reference, EXTERNAL_MODULE, strlen(EXTERNAL_MODULE)) && reference[strlen(EXTERNAL_MODULE)]=='\'') {
      return ObjectInfo(okExternal);
   }
   else {
      ref_t referenceID = mapReference(reference, existing);

      return defineObjectInfo(referenceID);
   }
}

ref_t Compiler::ModuleScope :: loadClassInfo(ClassInfo& info, const wchar16_t* vmtName)
{
   _Module* argModule;

   return loadClassInfo(argModule, info, vmtName);
}

ref_t Compiler::ModuleScope :: loadClassInfo(_Module* &argModule, ClassInfo& info, const wchar16_t* vmtName)
{
   if (emptystr(vmtName))
      return 0;

   // load class meta data
   ref_t moduleRef = 0;
   argModule = project->resolveModule(vmtName, moduleRef);

   if (argModule == NULL || moduleRef == 0)
      return 0;

   // load argument VMT meta data
   _Memory* metaData = argModule->mapSection(moduleRef | mskMetaRDataRef, true);
   if (metaData == NULL)
      return 0;

   MemoryReader reader(metaData);

   info.load(&reader);

   if (argModule != module) {
      // import class class reference
      if (info.classClassRef != 0)
         info.classClassRef = importReference(argModule, info.classClassRef, module);

      // import reference
      importReference(argModule, moduleRef, module);

      // import type
      info.header.typeRef = importSubject(argModule, info.header.typeRef, module);
   }
   return moduleRef;
}

bool Compiler::ModuleScope :: checkTypeMethod(ref_t type_ref, ref_t message)
{
   ref_t reference = typeHints.get(type_ref);
   if (reference) {
      _Module* extModule;
      ClassInfo info;
      loadClassInfo(extModule, info, module->resolveReference(reference));

      if (extModule) {
         if (module != extModule) {
            int   verb, paramCount;
            ref_t subj;
            decodeMessage(message, subj, verb, paramCount);
            if (subj != 0) {
               ref_t extSubj = extModule->mapSubject(module->resolveSubject(subj), true);
               if (extSubj != 0) {
                  return info.methods.exist(encodeMessage(extSubj, verb, paramCount));
               }
               else return false;
            }
            else return info.methods.exist(message);
         }
         else return info.methods.exist(message);
      }
   }

   return false;
}

bool Compiler::ModuleScope :: checkTypeMethod(ref_t type_ref, const wchar16_t* message, int verb, int paramCount)
{
   ref_t reference = typeHints.get(type_ref);
   if (reference) {
      _Module* extModule;
      ClassInfo info;
      loadClassInfo(extModule, info, module->resolveReference(reference));

      if (extModule) {
         ref_t msg = extModule->mapSubject(message, true);
         if (msg) {
            return info.methods.exist(encodeMessage(msg, verb, paramCount));
         }
      }
   }

   return false;
}

void Compiler::ModuleScope :: validateReference(TerminalInfo terminal, ref_t reference)
{
   // check if the reference may be resolved
   bool found = false;

   if (warnOnUnresolved && (warnOnWeakUnresolved || !isWeakReference(terminal))) {
      int   mask = reference & mskAnyRef;
      reference &= ~mskAnyRef;

      ref_t    ref = 0;
      _Module* refModule = project->resolveModule(module->resolveReference(reference), ref, true);

      if (refModule != NULL) {
         found = (refModule->mapSection(ref | mask, true)!=NULL);
      }
      if (!found) {
         if (!refModule || refModule == module) {
            forwardsUnresolved->add(Unresolved(sourcePath, reference | mask, module, terminal.Row(), terminal.Col()));
         }
         else raiseWarning(wrnUnresovableLink, terminal);
      }
   }
}

void Compiler::ModuleScope :: raiseError(const char* message, TerminalInfo terminal)
{
   project->raiseError(message, sourcePath, terminal.Row(), terminal.Col(), terminal.value);
}

void Compiler::ModuleScope :: raiseWarning(const char* message, TerminalInfo terminal)
{
   project->raiseWarning(message, sourcePath, terminal.Row(), terminal.Col(), terminal.value);
}

void Compiler::ModuleScope :: compileForwardHints(DNode hints, bool& constant)
{
   constant = false;

   while (hints == nsHint) {
      if (ConstantIdentifier::compare(hints.Terminal(), HINT_CONSTANT)) {
         constant = true;
      }
      else raiseWarning(wrnUnknownHint, hints.Terminal());

      hints = hints.nextNode();
   }
}

void Compiler::ModuleScope :: loadTypes(_Module* extModule)
{
   if (extModule) {
      ReferenceNs sectionName(extModule->Name(), TYPE_SECTION);

      _Memory* section = extModule->mapSection(extModule->mapReference(sectionName, true) | mskMetaRDataRef, true);
      if (section) {
         MemoryReader metaReader(section);
         while (!metaReader.Eof()) {
            ref_t subj_ref = importSubject(extModule, metaReader.getDWord(), module);

            if (subj_ref) {
               ref_t wrapper_ref = importReference(extModule, metaReader.getDWord(), module);
               int size = metaReader.getDWord();

               typeHints.add(subj_ref, wrapper_ref, true);

               if (size != 0)
                  sizeHints.add(subj_ref, size, true);
            }
            // if it is a synonym
            else {
               ref_t typeRef = importSubject(extModule, metaReader.getDWord(), module);
               ref_t synonymRef = importSubject(extModule, metaReader.getDWord(), module);

               synonymHints.add(synonymRef, typeRef, true);
            }
         }
      }
   }
}

void Compiler::ModuleScope :: loadExtensions(TerminalInfo terminal, _Module* extModule)
{
   if (extModule) {
      ReferenceNs sectionName(extModule->Name(), EXTENSION_SECTION);

      _Memory* section = extModule->mapSection(extModule->mapReference(sectionName, true) | mskMetaRDataRef, true);
      if (section) {
         MemoryReader metaReader(section);
         while (!metaReader.Eof()) {
            ref_t type_ref = importSubject(extModule, metaReader.getDWord(), module);
            ref_t message = importMessage(extModule, metaReader.getDWord(), module);
            ref_t role_ref = importReference(extModule, metaReader.getDWord(), module);

            if(!extensionHints.exist(message, type_ref)) {
               extensionHints.add(message, type_ref);

               SubjectMap* typeExtensions = extensions.get(type_ref);
               if (!typeExtensions) {
                  typeExtensions = new SubjectMap();

                  extensions.add(type_ref, typeExtensions);
               }

               typeExtensions->add(message, role_ref);
            }
            else raiseWarning(wrnDuplicateExtension, terminal);
         }
      }
   }
}

bool Compiler::ModuleScope :: saveType(ref_t type_ref, ref_t wrapper_ref, int size)
{
   ReferenceNs sectionName(module->Name(), TYPE_SECTION);

   MemoryWriter metaWriter(module->mapSection(mapReference(sectionName, false) | mskMetaRDataRef, false));

   metaWriter.writeDWord(type_ref);
   metaWriter.writeDWord(wrapper_ref);
   metaWriter.writeDWord(size);

   if (size != 0)
      sizeHints.add(type_ref, size);

   return typeHints.add(type_ref, wrapper_ref, true);
}

bool Compiler::ModuleScope :: saveSynonym(ref_t type_ref, ref_t synonym_ref)
{
   ReferenceNs sectionName(module->Name(), TYPE_SECTION);

   MemoryWriter metaWriter(module->mapSection(mapReference(sectionName, false) | mskMetaRDataRef, false));

   metaWriter.writeDWord(0);
   metaWriter.writeDWord(type_ref);
   metaWriter.writeDWord(synonym_ref);

   return synonymHints.add(synonym_ref, type_ref, true);
}

bool Compiler::ModuleScope :: saveExtension(ref_t message, ref_t type, ref_t role)
{
   ReferenceNs sectionName(module->Name(), EXTENSION_SECTION);

   MemoryWriter metaWriter(module->mapSection(mapReference(sectionName, false) | mskMetaRDataRef, false));

   metaWriter.writeDWord(type);
   metaWriter.writeDWord(message);
   metaWriter.writeDWord(role);

   if (!extensionHints.exist(message, type)) {
      extensionHints.add(message, type);

      SubjectMap* typeExtensions = extensions.get(type);
      if (!typeExtensions) {
         typeExtensions = new SubjectMap();

         extensions.add(type, typeExtensions);
      }

      typeExtensions->add(message, role);

      return true;
   }
   else return false;
}

ref_t Compiler::ModuleScope :: getClassType(ref_t reference)
{
   ClassInfo info;

   if(loadClassInfo(info, module->resolveReference(reference))) {
      return info.header.typeRef;
   }
   else return 0;
}

ref_t Compiler::ModuleScope :: getBoolType()
{
   if (!boolType) {
      ClassInfo info;

      if(loadClassInfo(info, ConstantIdentifier(TRUE_CLASS))) {
         boolType = info.header.typeRef;
      }
   }

   return boolType;
}

ref_t Compiler::ModuleScope :: getIntType()
{
   if (!intType) {
      ClassInfo info;

      if(loadClassInfo(info, ConstantIdentifier(INT_CLASS))) {
         intType = info.header.typeRef;
      }
   }

   return intType;
}

ref_t Compiler::ModuleScope :: getLongType()
{
   if (!longType) {
      ClassInfo info;

      if(loadClassInfo(info, ConstantIdentifier(LONG_CLASS))) {
         longType = info.header.typeRef;
      }
   }

   return longType;
}

ref_t Compiler::ModuleScope :: getRealType()
{
   if (!realType) {
      ClassInfo info;

      if(loadClassInfo(info, ConstantIdentifier(REAL_CLASS))) {
         realType = info.header.typeRef;
      }
   }

   return realType;
}

ref_t Compiler::ModuleScope :: getLiteralType()
{
   if (!literalType) {
      ClassInfo info;

      if(loadClassInfo(info, ConstantIdentifier(WSTR_CLASS))) {
         literalType = info.header.typeRef;
      }
   }

   return literalType;
}

ref_t Compiler::ModuleScope :: getParamsType()
{
   return paramsType;
}

ref_t Compiler::ModuleScope :: getActionType()
{
   return actionType;
}

// --- Compiler::SourceScope ---

//Compiler::SourceScope :: SourceScope(Scope* parent)
//   : Scope(parent)
//{
//   this->reference = 0;
//}

Compiler::SourceScope :: SourceScope(ModuleScope* moduleScope, ref_t reference)
   : Scope(moduleScope)
{
   this->reference = reference;
}

// --- Compiler::SymbolScope ---

Compiler::SymbolScope :: SymbolScope(ModuleScope* parent, ref_t reference)
   : SourceScope(parent, reference)
{
//   param = NULL;
   typeRef = 0;
}

void Compiler::SymbolScope :: compileHints(DNode hints, bool& constant)
{
   while (hints == nsHint) {
      TerminalInfo terminal = hints.Terminal();

      if (ConstantIdentifier::compare(terminal, HINT_TYPE)) {
         DNode value = hints.select(nsHintValue);

         if (value!=nsNone) {
            typeRef = moduleScope->mapSubject(value.Terminal());
         }
         else raiseWarning(wrnInvalidHint, terminal);
      }
      else if (ConstantIdentifier::compare(terminal, HINT_CONSTANT)) {
         constant = true;
      }
      else raiseWarning(wrnUnknownHint, hints.Terminal());

      hints = hints.nextNode();
   }
}

ObjectInfo Compiler::SymbolScope :: mapObject(TerminalInfo identifier)
{
   /*if (StringHelper::compare(identifier, param) || ConstIdentifier::compare(identifier, PARAM_VAR)) {
      return ObjectInfo(otLocal, -1);
   }
   else */return Scope::mapObject(identifier);
}

// --- Compiler::ClassScope ---

Compiler::ClassScope :: ClassScope(ModuleScope* parent, ref_t reference)
   : SourceScope(parent, reference)
{
   info.header.typeRef = 0;
   info.header.parentRef = moduleScope->mapConstantReference(SUPER_CLASS);
   info.header.flags = elStandartVMT;
   info.size = 0;
   info.classClassRef = 0;
   extensionTypeRef = 0;
}

ObjectInfo Compiler::ClassScope :: mapObject(TerminalInfo identifier)
{
   if (ConstantIdentifier::compare(identifier, SUPER_VAR)) {
      return ObjectInfo(okSuper, info.header.parentRef);
   }
   else if (ConstantIdentifier::compare(identifier, SELF_VAR)) {
      return ObjectInfo(okParam, -1);
   }
   else {
      int reference = info.fields.get(identifier);
      if (reference != -1) {
         if (test(info.header.flags, elStructureRole)) {
            int offset = reference;

            // HOTFIX : if it is a first data field, $self can be used instead
            if (offset == 0) {
               return ObjectInfo(okLocal, 1, info.fieldTypes.get(offset));
            }
            else return ObjectInfo(okFieldAddress, offset, info.fieldTypes.get(offset));
         }
//         else if (test(info.header.flags, elDynamicRole)) {
//            int type = getClassType();
//            if (type == elDebugArray) {
//               return ObjectInfo(okField, otArray, -1);
//            }
//            else return ObjectInfo(okUnknown);
//         }
         // otherwise it is a normal field
         else return ObjectInfo(okField, reference, info.fieldTypes.get(reference));
      }
      else return Scope::mapObject(identifier);
   }
}

void Compiler::ClassScope :: compileClassHints(DNode hints)
{
   // define class flags
   while (hints == nsHint) {
      TerminalInfo terminal = hints.Terminal();

      if (ConstantIdentifier::compare(terminal, HINT_TYPE)) {
         DNode value = hints.select(nsHintValue);

         if (value!=nsNone) {
            info.header.typeRef = moduleScope->mapSubject(value.Terminal());
         }
         else raiseWarning(wrnInvalidHint, terminal);
      }
      else if (ConstantIdentifier::compare(terminal, HINT_GROUP)) {
         info.header.flags |= elGroup;
      }
      else if (ConstantIdentifier::compare(terminal, HINT_MESSAGE)) {
         info.header.flags |= elMessage;
      }
      else if (ConstantIdentifier::compare(terminal, HINT_SIGNATURE)) {
         info.header.flags |= elSignature;
      }
      else if (ConstantIdentifier::compare(terminal, HINT_ROLE)) {
         info.header.flags |= elRole;
      }
      else if (ConstantIdentifier::compare(terminal, HINT_EXTENSION)) {
         info.header.flags |= elExtension;
         DNode value = hints.select(nsHintValue);
         if (value != nsNone) {
            extensionTypeRef = moduleScope->mapSubject(value.Terminal());
         }
      }      
      else if (ConstantIdentifier::compare(terminal, HINT_SEALED)) {
         info.header.flags |= elSealed;
      }
      else if (ConstantIdentifier::compare(terminal, HINT_DBG)) {
         TerminalInfo value = hints.select(nsHintValue).Terminal();

         if (ConstantIdentifier::compare(value, HINT_DBG_INT)) {
            info.header.flags |= elDebugDWORD;
         }
         else if (ConstantIdentifier::compare(value, HINT_DBG_LONG)) {
            info.header.flags |= elDebugQWORD;
         }
         else if (ConstantIdentifier::compare(value, HINT_DBG_LITERAL)) {
            info.header.flags |= elDebugLiteral;
         }
         else if (ConstantIdentifier::compare(value, HINT_DBG_REAL)) {
            info.header.flags |= elDebugReal64;
         }
         else if (ConstantIdentifier::compare(value, HINT_DBG_ARRAY)) {
            info.header.flags |= elDebugArray;
         }
         else raiseWarning(wrnInvalidHint, value);
      }
      else raiseWarning(wrnUnknownHint, terminal);

      hints = hints.nextNode();
   }

}

//int Compiler::ClassScope :: getFieldSizeHint()
//{
//   switch (info.header.flags & elDebugMask) {
//      case elDebugDWORD:
//         return 4;
//      case elDebugReal64:
//      case elDebugQWORD:
//         return 8;
//      case elDebugArray:
//         return (size_t)-4;
//      case elDebugLiteral:
//         return (size_t)-2;
//      case elDebugBytes:
//         return (size_t)-1;
//      default:
//         return 0;
//   }
//}
//
//int Compiler::ClassScope :: getFieldSizeHint(TerminalInfo terminal)
//{
//   if (terminal.symbol == tsInteger) {
//      return StringHelper::strToInt(terminal);
//   }
//   else if (terminal.symbol == tsHexInteger) {
//      return StringHelper::strToLong(terminal, 16);
//   }
//   else {
//      raiseWarning(wrnUnknownHint, terminal);
//
//      return 0;
//   }
//}

void Compiler::ClassScope :: compileFieldHints(DNode hints, int& size, ref_t& type)
{
   while (hints == nsHint) {
      TerminalInfo terminal = hints.Terminal();

      if (ConstantIdentifier::compare(terminal, HINT_SIZE)) {
         if (info.fields.Count() > 0)
            raiseError(wrnInvalidHint, terminal);

         TerminalInfo sizeValue = hints.select(nsHintValue).Terminal();
         if (sizeValue.symbol == tsInteger) {
            size = align(StringHelper::strToInt(sizeValue.value), 4);
         }
         else raiseWarning(wrnUnknownHint, terminal);
      }
      else if (ConstantIdentifier::compare(terminal, HINT_ITEMSIZE)) {
         if (info.fields.Count() > 0)
            raiseError(wrnInvalidHint, terminal);

         TerminalInfo sizeValue = hints.select(nsHintValue).Terminal();
         if (sizeValue.symbol == tsInteger) {
            size = -StringHelper::strToInt(sizeValue.value);
         }
         else raiseWarning(wrnUnknownHint, terminal);
      }
      else if (ConstantIdentifier::compare(terminal, HINT_ITEM)) {
         if (info.fields.Count() > 0)
            raiseError(wrnInvalidHint, terminal);

         size = -4;
         type = -1;
      }
      else if (ConstantIdentifier::compare(terminal, HINT_TYPE)) {
         DNode value = hints.select(nsHintValue);
         if (value!=nsNone) {
            TerminalInfo typeTerminal = value.Terminal();

            type = moduleScope->mapSubject(typeTerminal);
            size = moduleScope->sizeHints.get(type);
         }
         else raiseWarning(wrnInvalidHint, terminal);
      }

      hints = hints.nextNode();
   }
}

// --- Compiler::MetodScope ---

Compiler::MethodScope :: MethodScope(ClassScope* parent)
   : Scope(parent), parameters(Parameter())
{
   this->message = 0;
   this->withBreakHandler = false;
//   this->withCustomVerb = false;
   this->reserved = 0;
   this->masks = 0;
   this->rootToFree = 1;
}

void Compiler::MethodScope :: include()
{
   ClassScope* classScope = (ClassScope*)getScope(Scope::slClass);

   // check if the method is inhreited and update vmt size accordingly
   ClassInfo::MethodMap::Iterator it = classScope->info.methods.getIt(message);
   if (it.Eof()) {
      classScope->info.methods.add(message, true);
   }
   else (*it) = true;
}

ObjectInfo Compiler::MethodScope :: mapObject(TerminalInfo identifier)
{
   if (ConstantIdentifier::compare(identifier, THIS_VAR)) {
      return ObjectInfo(okThisParam, 1, getClassType());
   }
   else if (ConstantIdentifier::compare(identifier, SELF_VAR)) {
      ObjectInfo retVal = parent->mapObject(identifier);
      // overriden to set FRAME USED flag
      if (retVal.kind == okParam) {
         masks |= MTH_FRAME_USED;
      }

      return retVal;
   }
   else {
      Parameter param = parameters.get(identifier);

      int local = param.offset;
      if (local >= 0) {
         masks |= MTH_FRAME_USED;

         if (param.sign_ref == moduleScope->getParamsType()) {
            return ObjectInfo(okParams, -1 - local, param.sign_ref);
         }
         else return ObjectInfo(param.out ? okLocal : okParam, -1 - local, param.sign_ref);
      }
      else {
         ObjectInfo retVal = Scope::mapObject(identifier);

         return retVal;
      }
   }
}

int Compiler::MethodScope :: compileHints(DNode hints)
{
   int mode = 0;

   while (hints == nsHint) {
      TerminalInfo terminal = hints.Terminal();

      raiseWarning(wrnUnknownHint, terminal);

      hints = hints.nextNode();
   }
   return mode;
}

// --- Compiler::ActionScope ---

Compiler::ActionScope :: ActionScope(ClassScope* parent)
   : MethodScope(parent)
{
}

ObjectInfo Compiler::ActionScope :: mapObject(TerminalInfo identifier)
{
   // action does not support this variable
   if (ConstantIdentifier::compare(identifier, THIS_VAR)) {
      return parent->mapObject(identifier);
   }
   else return MethodScope::mapObject(identifier);
}

// --- Compiler::CodeScope ---

Compiler::CodeScope :: CodeScope(SourceScope* parent)
   : Scope(parent), locals(Parameter(0))
{
   this->tape = &parent->tape;
   this->level = 0;
   this->saved = this->reserved = 0;
}

//Compiler::CodeScope :: CodeScope(MethodScope* parent, CodeType type)
//   : Scope(parent)
//{
//   this->tape = &((ClassScope*)parent->getScope(slClass))->tape;
//   this->level = 0;
//}

Compiler::CodeScope :: CodeScope(MethodScope* parent)
   : Scope(parent), locals(Parameter(0))
{
   this->tape = &((ClassScope*)parent->getScope(slClass))->tape;
   this->level = 0;
   this->saved = this->reserved = 0;
}

Compiler::CodeScope :: CodeScope(CodeScope* parent)
   : Scope(parent), locals(Parameter(0))
{
   this->tape = parent->tape;
   this->level = parent->level;
   this->saved = parent->saved;
   this->reserved = parent->reserved;
}

ObjectInfo Compiler::CodeScope :: mapObject(TerminalInfo identifier)
{
   Parameter local = locals.get(identifier);
   if (local.offset) {
      return ObjectInfo(okLocal, local.offset, local.sign_ref);
   }
   else return Scope::mapObject(identifier);
}

void Compiler::CodeScope :: compileLocalHints(DNode hints, ref_t& type, int& size)
{
   while (hints == nsHint) {
      TerminalInfo terminal = hints.Terminal();

      if (ConstantIdentifier::compare(terminal, HINT_TYPE)) {
         TerminalInfo typeValue = hints.firstChild().Terminal();

         type = moduleScope->mapType(typeValue);
         if (!type)
            raiseError(errUnknownSubject, typeValue);

         size = moduleScope->sizeHints.get(type);
      }
      else if (ConstantIdentifier::compare(terminal, HINT_SIZE)) {
         int itemSize = moduleScope->sizeHints.get(type);

         TerminalInfo sizeValue = hints.firstChild().Terminal();
         if (itemSize < 0 && sizeValue.symbol == tsInteger) {
            itemSize = -itemSize;

            size = StringHelper::strToInt(sizeValue.value) * itemSize;
         }
         else if (itemSize < 0 && sizeValue.symbol == tsHexInteger) {
            itemSize = -itemSize;

            size = StringHelper::strToLong(sizeValue.value, 16) * itemSize;
         }
         else raiseWarning(wrnUnknownHint, terminal);
      }
      else raiseWarning(wrnUnknownHint, terminal);

      hints = hints.nextNode();
   }
}

// --- Compiler::InlineClassScope ---

Compiler::InlineClassScope :: InlineClassScope(CodeScope* owner, ref_t reference)
   : ClassScope(owner->moduleScope, reference), outers(Outer())
{
   this->parent = owner;
   info.header.flags |= elNestedClass;
}

Compiler::InlineClassScope::Outer Compiler::InlineClassScope :: mapSelf()
{
   String<wchar16_t, 10> thisVar(THIS_VAR);

   Outer owner = outers.get(thisVar);
   // if owner reference is not yet mapped, add it
   if (owner.outerObject.kind == okUnknown) {
      owner.reference = info.fields.Count();
      owner.outerObject.kind = okThisParam;
      owner.outerObject.param = 1;

      outers.add(thisVar, owner);
      mapKey(info.fields, thisVar, owner.reference);
   }
   return owner;
}

ObjectInfo Compiler::InlineClassScope :: mapObject(TerminalInfo identifier)
{
   if (ConstantIdentifier::compare(identifier, THIS_VAR)) {
      //return ObjectInfo(okSelf, 0);
      Outer owner = mapSelf();

      // map as an outer field (reference to outer object and outer object field index)
      return ObjectInfo(okOuter, owner.reference);
   }
   else {
      Outer outer = outers.get(identifier);

	  // if object already mapped
      if (outer.reference!=-1) {
         if (outer.outerObject.kind == okSuper) {
            return ObjectInfo(okSuper, outer.reference);
         }
         else return ObjectInfo(okOuter, outer.reference);
      }
      else {
         outer.outerObject = parent->mapObject(identifier);
         // handle outer fields in a special way: save only self
         if (outer.outerObject.kind==okField) {
            Outer owner = mapSelf();

            // map as an outer field (reference to outer object and outer object field index)
            return ObjectInfo(okOuterField, owner.reference, outer.outerObject.param);
         }
         // map if the object is outer one
         else if (outer.outerObject.kind==okParam || outer.outerObject.kind==okLocal || outer.outerObject.kind==okField 
            || outer.outerObject.kind==okOuter || outer.outerObject.kind==okSuper || outer.outerObject.kind == okThisParam
            || outer.outerObject.kind==okOuterField)
         {
            outer.reference = info.fields.Count();

            outers.add(identifier, outer);
            mapKey(info.fields, identifier.value, outer.reference);

            return ObjectInfo(okOuter, outer.reference);
         }
         // if inline symbol declared in symbol it treats self variable in a special way
         else if (ConstantIdentifier::compare(identifier, SELF_VAR)) {
            return ObjectInfo(okParam, -1);
         }
         else if (outer.outerObject.kind == okUnknown) {
            // check if there is inherited fields
            outer.reference = info.fields.get(identifier);
            if (outer.reference != -1) {
               return ObjectInfo(okField, outer.reference);
            }
            else return outer.outerObject;
         }
         else return outer.outerObject;
      }
   }
}

// --- Compiler ---

Compiler :: Compiler(StreamReader* syntax)
   : _parser(syntax), _verbs(0)
{
   ByteCodeCompiler::loadVerbs(_verbs);
   ByteCodeCompiler::loadOperators(_operators);

   // default settings
   _optFlag = 0;
}

void Compiler :: loadRules(StreamReader* optimization)
{
   _rules.load(optimization);
}

bool Compiler :: optimizeJumps(CommandTape& tape)
{
   //if (!test(_optFlag, optJumps))
   //   return false;

   return CommandTape::optimizeJumps(tape);
}

bool Compiler :: applyRules(CommandTape& tape)
{
   if (!_rules.loaded)
      return false;

   if (_rules.apply(tape)) {
      while (_rules.apply(tape));

      return true;
   }
   else return false;
}

void Compiler :: optimizeTape(CommandTape& tape)
{
   // optimize unsued and idle jumps
   while (optimizeJumps(tape));

   // optimize the code 
   bool modified = false;
   while (applyRules(tape)) { 
      modified = true; 
   }

   if (modified) {
      optimizeTape(tape);
   }
}

ref_t Compiler :: mapNestedExpression(CodeScope& scope, int mode, ref_t& typeRef)
{
   ModuleScope* moduleScope = scope.moduleScope;

   // if it is a root inline expression we could try to name it after the symbol
   if (test(mode, HINT_ROOT)) {
      // check if the inline symbol is declared in the symbol
      SymbolScope* symbol = (SymbolScope*)scope.getScope(Scope::slSymbol);
      if (symbol != NULL) {
         typeRef = symbol->typeRef;
         return symbol->reference;
      }
   }

   // otherwise auto generate the name
   ReferenceNs name(moduleScope->module->Name(), INLINE_POSTFIX);

   findUninqueName(moduleScope->module, name);

   return moduleScope->module->mapReference(name);
}

void Compiler :: declareParameterDebugInfo(MethodScope& scope, CommandTape* tape, bool withSelf)
{
   ModuleScope* moduleScope = scope.moduleScope;

   // declare method parameter debug info
   LocalMap::Iterator it = scope.parameters.start();
   while (!it.Eof()) {
      int size = moduleScope->sizeHints.get((*it).sign_ref);

      if ((*it).sign_ref == moduleScope->getParamsType()) {
         _writer.declareLocalParamsInfo(*tape, it.key(), -1 - (*it).offset);
      }
      else if ((*it).sign_ref == moduleScope->getIntType() || size == 4 || size == 2) {
         _writer.declareLocalIntInfo(*tape, it.key(), -1 - (*it).offset);
      }
      else if ((*it).sign_ref == moduleScope->getLongType()) {
         _writer.declareLocalLongInfo(*tape, it.key(), -1 - (*it).offset);
      }
      else if ((*it).sign_ref == moduleScope->getRealType()) {
         _writer.declareLocalRealInfo(*tape, it.key(), -1 - (*it).offset);
      }
      else _writer.declareLocalInfo(*tape, it.key(), -1 - (*it).offset);

      it++;
   }
   if (withSelf)
      _writer.declareSelfInfo(*tape, 1);
}

void Compiler :: importCode(DNode node, ModuleScope& scope, CommandTape* tape, const wchar16_t* referenceName)
{
   ByteCodeIterator it = tape->end();

   ref_t reference = 0;
   _Module* api = scope.project->resolveModule(referenceName, reference);

   _Memory* section = api != NULL ? api->mapSection(reference | mskCodeRef, true) : NULL;
   if (section == NULL) {
      scope.raiseError(errInvalidLink, node.Terminal());
   }
   else tape->import(section);

   // goes to the first imported command
   it++;

   // import references
   while (!it.Eof()) {
      CommandTape::import(*it, api, scope.module);
      it++;
   }
}

Compiler::InheritResult Compiler :: inheritClass(ClassScope& scope, ref_t parentRef)
{
   ModuleScope* moduleScope = scope.moduleScope;

   size_t flagCopy = scope.info.header.flags;
   size_t classClassCopy = scope.info.classClassRef;
   size_t typeCopy = scope.info.header.typeRef;

   // get module reference
   ref_t moduleRef = 0;
   _Module* module = moduleScope->project->resolveModule(moduleScope->module->resolveReference(parentRef), moduleRef);

   if (module == NULL || moduleRef == 0)
      return irUnsuccessfull;

   // load parent meta data
   _Memory* metaData = module->mapSection(moduleRef | mskMetaRDataRef, true);
   if (metaData != NULL) {
      MemoryReader reader(metaData);
      // import references if we inheriting class from another module
      if (moduleScope->module != module) {
         ClassInfo copy;
         copy.load(&reader);

         scope.info.header = copy.header;
         scope.info.size = copy.size;

         // import type subject
         scope.info.header.typeRef = importSubject(module, copy.header.typeRef, moduleScope->module);

         // import method references and mark them as inherited
         ClassInfo::MethodMap::Iterator it = copy.methods.start();
         while (!it.Eof()) {
            scope.info.methods.add(importMessage(module, it.key(), moduleScope->module), false);

            it++;
         }

         scope.info.fields.add(copy.fields);

         ClassInfo::FieldTypeMap::Iterator type_it = copy.fieldTypes.start();
         while (!type_it.Eof()) {
            scope.info.fieldTypes.add(type_it.key(), importSubject(module, *type_it, moduleScope->module));

            type_it++;
         }
      }
      else {
         scope.info.load(&reader);

         // mark all methods as inherited
         ClassInfo::MethodMap::Iterator it = scope.info.methods.start();
         while (!it.Eof()) {
            (*it) = false;

            it++;
         }
      }

      if (test(scope.info.header.flags, elSealed))
         return irSealed;

      // restore parent and flags
      scope.info.header.parentRef = parentRef;
      scope.info.classClassRef = classClassCopy;
      scope.info.header.flags |= flagCopy;
      if (typeCopy != 0) 
         scope.info.header.typeRef = typeCopy;

      return irSuccessfull;
   }
   else return irUnsuccessfull;
}

void Compiler :: compileParentDeclaration(DNode node, ClassScope& scope)
{
   // base class system'object must not to have a parent
   ref_t parentRef = scope.info.header.parentRef;
   if (scope.info.header.parentRef == scope.reference) {
      if (node.Terminal() != nsNone)
         scope.raiseError(errInvalidSyntax, node.Terminal());

      parentRef = 0;
   }
   // if the class has an implicit parent
   else if (node.Terminal() != nsNone) {
      TerminalInfo identifier = node.Terminal();
      if (identifier == tsIdentifier || identifier == tsPrivate) {
         parentRef = scope.moduleScope->mapTerminal(node.Terminal(), true);
      }
      else parentRef = scope.moduleScope->mapReference(identifier);

      if (parentRef == 0)
         scope.raiseError(errUnknownClass, node.Terminal());
   }
   InheritResult res = compileParentDeclaration(parentRef, scope);
   //if (res == irObsolete) {
   //   scope.raiseWarning(wrnObsoleteClass, node.Terminal());
   //}
   if (res == irInvalid) {
      scope.raiseError(errInvalidParent, node.Terminal());
   }
   if (res == irSealed) {
      scope.raiseError(errSealedParent, node.Terminal());
   }
   else if (res == irUnsuccessfull)
      scope.raiseError(node != nsNone ? errUnknownClass : errUnknownBaseClass, node.Terminal());
}

Compiler::InheritResult Compiler :: compileParentDeclaration(ref_t parentRef, ClassScope& scope)
{
   scope.info.header.parentRef = parentRef;
   if (scope.info.header.parentRef != 0) {
      return inheritClass(scope, scope.info.header.parentRef);
   }
   else return irSuccessfull;
}

void Compiler :: compileSwitch(DNode node, CodeScope& scope, ObjectInfo switchValue)
{
   _writer.declareSwitchBlock(*scope.tape);

   if (switchValue.kind == okAccumulator) {
      _writer.pushObject(*scope.tape, switchValue);

      switchValue.kind = okBlockLocal;
      switchValue.param = 1;
   }

   DNode option = node.firstChild();
   while (option == nsSwitchOption || option == nsBiggerSwitchOption || option == nsLessSwitchOption)  {
      _writer.declareSwitchOption(*scope.tape);

      int operator_id = EQUAL_MESSAGE_ID;
      if (option == nsBiggerSwitchOption) {
         operator_id = GREATER_MESSAGE_ID;
      }
      else if (option == nsLessSwitchOption) {
         operator_id = LESS_MESSAGE_ID;
      }

      ObjectInfo optionValue = compileObject(option.firstChild(), scope, 0);
      _writer.loadObject(*scope.tape, optionValue);

      if (checkIfBoxingRequired(scope, optionValue))
         boxObject(scope, optionValue, 0);

      _writer.pushObject(*scope.tape, ObjectInfo(okAccumulator));
      _writer.pushObject(*scope.tape, switchValue);

      _writer.setMessage(*scope.tape, encodeMessage(0, operator_id, 1));
      _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 0));
      _writer.callMethod(*scope.tape, 0, 1);

      bool mismatch = false;
      compileTypecast(scope, ObjectInfo(okAccumulator), scope.moduleScope->getBoolType(), mismatch, HINT_TYPEENFORCING);

      _writer.jumpIfEqual(*scope.tape, scope.moduleScope->falseReference);

      CodeScope subScope(&scope);
      DNode thenCode = option.firstChild().nextNode();

      _writer.declareBlock(*scope.tape);

      if (thenCode.firstChild().nextNode() != nsNone) {
         compileCode(thenCode, subScope, HINT_SUBBRANCH);
      }
      // if it is inline action
      else compileRetExpression(thenCode.firstChild(), scope, 0);

      _writer.endSwitchOption(*scope.tape);

      option = option.nextNode();
   }
   if (option == nsLastSwitchOption) {
      CodeScope subScope(&scope);
      DNode thenCode = option.firstChild();

      _writer.declareBlock(*scope.tape);

      if (thenCode.firstChild().nextNode() != nsNone) {
         compileCode(thenCode, subScope);
      }
      // if it is inline action
      else compileRetExpression(thenCode.firstChild(), scope, 0);
   }

   _writer.endSwitchBlock(*scope.tape);
}

void Compiler :: compileAssignment(DNode node, CodeScope& scope, ObjectInfo object)
{
   if (object.kind == okLocal || object.kind == okField) {
      _writer.saveObject(*scope.tape, object);
   }
   else if ((object.kind == okOuter)) {
      scope.raiseWarning(wrnOuterAssignment, node.Terminal());
      _writer.saveObject(*scope.tape, object);
   }
   else if (object.kind == okUnknown) {
      scope.raiseError(errUnknownObject, node.Terminal());
   }
   else scope.raiseError(errInvalidOperation, node.Terminal());
}

void Compiler :: compileContentAssignment(DNode node, CodeScope& scope, ObjectInfo variableInfo, ObjectInfo object)
{
   if (variableInfo.kind == okLocal || variableInfo.kind == okFieldAddress) {
      if (object.kind == okIndexAccumulator) {
         if (variableInfo.extraparam == scope.moduleScope->getIntType()) {
            _writer.saveInt(*scope.tape, variableInfo);
         }
         else scope.raiseError(errInvalidOperation, node.Terminal());
      }
      else {
         int size = scope.moduleScope->sizeHints.get(variableInfo.extraparam);
         if (size == 4) {
            _writer.assignInt(*scope.tape, variableInfo);
         }
         else if (size == 2) {
            _writer.assignShort(*scope.tape, variableInfo);
         }
         else if (size == 8) {
            _writer.assignLong(*scope.tape, variableInfo);
         }
         else scope.raiseError(errInvalidOperation, node.Terminal());
      }
   }
   else scope.raiseError(errInvalidOperation, node.Terminal());
}

void Compiler :: compileVariable(DNode node, CodeScope& scope, DNode hints)
{
   if (!scope.locals.exist(node.Terminal())) {
      ref_t type = 0;
      int size = 0;
      scope.compileLocalHints(hints, type, size);

      bool stackAllocated = (type != 0 && scope.moduleScope->typeHints.get(type) != 0);

      int level = scope.newLocal();

      if (stackAllocated) {
         ObjectInfo primitive(okLocal, 0, type);

         if(!allocateStructure(scope, size, primitive))
            scope.raiseError(errInvalidOperation, node.Terminal());

         // make the reservation permanent
         scope.saved = scope.reserved;

         _writer.pushObject(*scope.tape, primitive);
         if (size == 4 || size == 2) {
            _writer.declareLocalIntInfo(*scope.tape, node.Terminal(), level);
         }
         else if (type == scope.moduleScope->getLongType()) {
            _writer.declareLocalLongInfo(*scope.tape, node.Terminal(), level);
         }
         else if (type == scope.moduleScope->getRealType()) {
            _writer.declareLocalRealInfo(*scope.tape, node.Terminal(), level);
         }
         else if (scope.moduleScope->sizeHints.get(type) == -1) {
            _writer.declareLocalByteArrayInfo(*scope.tape, node.Terminal(), level);
         }
         else if (scope.moduleScope->sizeHints.get(type) == -2) {
            _writer.declareLocalShortArrayInfo(*scope.tape, node.Terminal(), level);
         }
         else _writer.declareLocalInfo(*scope.tape, node.Terminal(), level);
      }
      else {
         _writer.declareVariable(*scope.tape, scope.moduleScope->nilReference);
         _writer.declareLocalInfo(*scope.tape, node.Terminal(), level); 
      }

      DNode assigning = node.firstChild();
      if (assigning != nsNone) {
         if (stackAllocated) {
            scope.mapLocal(node.Terminal(), level, type);

            ObjectInfo info = compileExpression(assigning.firstChild(), scope, 0);
            if (info.kind == okIndexAccumulator) {
               // if it is a primitive operation
               compileContentAssignment(node, scope, scope.mapObject(node.Terminal()), info);
            }
            else {
               _writer.loadObject(*scope.tape, info);

               bool mismatch = false;
               compileTypecast(scope, info, type, mismatch, HINT_TYPEENFORCING);
               if (mismatch)
                  scope.raiseWarning(wrnTypeMismatch, node.Terminal());

               compileContentAssignment(node, scope, scope.mapObject(node.Terminal()), info);
            }
         }
         else {
            ObjectInfo info = compileExpression(assigning.firstChild(), scope, 0);

            scope.mapLocal(node.Terminal(), level, type);

            _writer.loadObject(*scope.tape, info);

            if (checkIfBoxingRequired(scope, info))
               info = boxObject(scope, info, 0);

            compileAssignment(node, scope, scope.mapObject(node.Terminal()));            
         }
      }
      else scope.mapLocal(node.Terminal(), level, type);
   }
   else scope.raiseError(errDuplicatedLocal, node.Terminal());

   MethodScope* methodScope = (MethodScope*)scope.getScope(Scope::slMethod);
   // indicate the frame usage
   // to prevent commenting frame operation out
   methodScope->masks = MTH_FRAME_USED;
}

ObjectInfo Compiler :: compileTerminal(DNode node, CodeScope& scope, int mode)
{
   TerminalInfo terminal = node.Terminal();

   ObjectInfo object;
   if (terminal==tsLiteral) {
      object = ObjectInfo(okLiteralConstant, scope.moduleScope->module->mapConstant(terminal));
   }
   else if (terminal == tsInteger) {
      int integer = StringHelper::strToInt(terminal.value);
      if (errno == ERANGE)
         scope.raiseError(errInvalidIntNumber, terminal);

      object = ObjectInfo(okIntConstant, scope.moduleScope->module->mapConstant(terminal));
   }
   else if (terminal == tsLong) {
      String<wchar16_t, 30> s("_"); // special mark to tell apart from integer constant
      s.append(terminal.value, getlength(terminal.value) - 1);

      long long integer = StringHelper::strToLongLong(s + 1, 10);
      if (errno == ERANGE)
         scope.raiseError(errInvalidIntNumber, terminal);

      object = ObjectInfo(okLongConstant, scope.moduleScope->module->mapConstant(s));
   }
   else if (terminal==tsHexInteger) {
      String<wchar16_t, 20> s(terminal.value, getlength(terminal.value) - 1);

      long integer = s.toLong(16);
      if (errno == ERANGE)
         scope.raiseError(errInvalidIntNumber, terminal);

      // convert back to string as a decimal integer
      s.clear();
      s.appendLong(integer);

      object = ObjectInfo(okIntConstant, scope.moduleScope->module->mapConstant(s));
   }
   else if (terminal == tsReal) {
      String<wchar16_t, 30> s(terminal.value, getlength(terminal.value) - 1);
      double number = StringHelper::strToDouble(s);
      if (errno == ERANGE)
         scope.raiseError(errInvalidIntNumber, terminal);

      object = ObjectInfo(okRealConstant, scope.moduleScope->module->mapConstant(s));
   }
   else if (!emptystr(terminal)) {
      object = scope.mapObject(terminal);
   }

   switch (object.kind) {
      case okUnknown:
         scope.raiseError(errUnknownObject, terminal);
         break;
      case okSymbol:
         scope.moduleScope->validateReference(terminal, object.param | mskSymbolRef);
         break;
      case okExternal:
         // external call cannot be used inside symbol
         if (test(mode, HINT_ROOT))
            scope.raiseError(errInvalidSymbolExpr, node.Terminal());
         break;
      case okFieldAddress:
         // field address cannot be used directly and should be boxed
         object = boxStructureField(scope, object, mode);
         break;
   }

   // skip the first breakpoint if it is not a symbol
   if (object.kind == okSymbol || !test(mode, HINT_ROOTEXPR))
      recordStep(scope, terminal, dsStep);

   return object;
}

ObjectInfo Compiler :: compileObject(DNode objectNode, CodeScope& scope, int mode)
{
   ObjectInfo result;

   DNode member = objectNode.firstChild();
   switch (member)
   {
      case nsRetStatement:
      case nsNestedClass:
         if (objectNode.Terminal() != nsNone) {
            result = compileNestedExpression(objectNode, scope, mode);
            break;
         }
      case nsSubCode:
      case nsSubjectArg:
      case nsMethodParameter:
         result = compileNestedExpression(member, scope, mode);
         break;
      case nsInlineExpression:
         result = compileNestedExpression(objectNode, scope, mode);
         break;
      case nsExpression:
         if (isCollection(member)) {
            TerminalInfo parentInfo = objectNode.Terminal();
            // if the parent class is defined
            if (parentInfo == tsIdentifier || parentInfo == tsReference || parentInfo == tsPrivate) {
               ref_t vmtReference = scope.moduleScope->mapTerminal(parentInfo, true);
               if (vmtReference == 0)
                  scope.raiseError(errUnknownObject, parentInfo);

               result = compileCollection(member, scope, mode & ~HINT_ROOT, vmtReference);
            }
            else result = compileCollection(member, scope, mode & ~HINT_ROOT);
         }
         else result = compileExpression(member, scope, mode & ~HINT_ROOT);
         break;
      case nsMessageReference:
         // if it is a message
         if (findSymbol(member.firstChild(), nsSizeValue, nsVarSizeValue)) {
            result = compileMessageReference(member, scope, mode);
         }
         // if it is a get property
         else if (findSymbol(member.firstChild(), nsExpression)) {
            return compileGetProperty(member, scope, mode);
         }
         // otherwise it is a singature
         else result = compileSignatureReference(member, scope, mode);
         break;
      case nsSymbolReference:
         result = compileReference(member, scope, mode);
         break;
      default:
         result = compileTerminal(objectNode, scope, mode);
   }

   return result;
}

ObjectInfo Compiler :: compileSignatureReference(DNode objectNode, CodeScope& scope, int mode)
{
   IdentifierString message;
   ref_t sign_id = 0;

   // reserve place for param counter
   message.append('0');

   // place dummy verb
   message.append('#');
   message.append(0x20);

   DNode arg = objectNode.firstChild();
   while (arg == nsSubjectArg) {
      TerminalInfo subject = arg.Terminal();

      message.append('&');
      message.append(subject);

      arg = arg.nextNode();
   }

   return ObjectInfo(okSignatureConstant, scope.moduleScope->module->mapReference(message));
}

ObjectInfo Compiler :: compileMessageReference(DNode objectNode, CodeScope& scope, int mode)
{
   DNode arg = objectNode.firstChild();
   TerminalInfo verb = arg.Terminal();

   IdentifierString message;

   // reserve place for param counter
   message.append('0');

   ref_t verb_id = _verbs.get(verb.value);
   ref_t sign_id = 0;
   int count = 0;

   // if it is not a verb - by default it is EVAL message
   if (verb_id == 0) {
      message.append('#');
      message.append(EVAL_MESSAGE_ID + 0x20);
      message.append('&');
      message.append(verb);
   }
   else {
      message.append('#');
      message.append(verb_id + 0x20);
   }

   // if it is a generic verb, make sure no parameters are provided
   if ((verb_id == DISPATCH_MESSAGE_ID) && objectNode.firstChild() == nsSubjectArg) {
      scope.raiseError(errInvalidOperation, verb);
   }

   arg = arg.nextNode();

   // if method has argument list
   while (arg == nsSubjectArg) {
      TerminalInfo subject = arg.Terminal();

      message.append('&');

      message.append(subject);

      count++;
      arg = arg.nextNode();
   }

   if (arg == nsSizeValue) {
      TerminalInfo size = arg.Terminal();
      if (size == tsInteger) {
         count = StringHelper::strToInt(size.value);
      }
      else scope.raiseError(errInvalidOperation, size);

      // if it is an open argument preceeded by normal ones
      if (arg.nextNode() == nsVarSizeValue) {
         message.append('&');
         message.appendInt(count);

         count = OPEN_ARG_COUNT;
      }
   }
   else if (arg == nsVarSizeValue) {
      count = OPEN_ARG_COUNT;
   }

   // if it is a custom verb and param count is zero - treat it like get message
   if (verb_id== 0 && count == 0) {
      message[2] += 1;
   }

   // define the number of parameters
   message[0] = message[0] + count;

   return ObjectInfo(okMessageConstant, scope.moduleScope->module->mapReference(message));
}

ObjectInfo Compiler :: compileReference(DNode objectNode, CodeScope& scope, int mode)
{
   ObjectInfo symbol = scope.mapObject(objectNode.Terminal());
   
   if (symbol.kind == okSymbol || symbol.kind == okConstantSymbol) {
      return ObjectInfo(okSymbolReference, symbol.param);
   }
   else scope.raiseError(errInvalidOperation, objectNode.Terminal());

   return ObjectInfo(okUnknown);
}

ObjectInfo Compiler :: saveObject(CodeScope& scope, ObjectInfo object, int mode)
{
   _writer.pushObject(*scope.tape, object);
   object.kind = okCurrent;

   return object;
}

bool Compiler :: checkIfBoxingRequired(CodeScope& scope, ObjectInfo object)
{
   // NOTE: boxing should be applied only for the typed local parameter
   if (object.kind == okParams) {
      return true;
   }
   else if (object.kind == okParam || object.kind == okLocal || object.kind == okLocalAddress || object.kind == okFieldAddress) {
      return scope.moduleScope->sizeHints.exist(object.extraparam);
   }
   else return false;
}

ObjectInfo Compiler :: boxObject(CodeScope& scope, ObjectInfo object, int mode)
{
   // NOTE: boxing should be applied only for the typed local parameter
   if (object.kind == okParams) {
      _writer.boxArgList(*scope.tape, scope.moduleScope->mapConstantReference(ARRAY_CLASS) | mskVMTRef);
   }
   else if (object.kind == okParam || object.kind == okLocal || object.kind == okLocalAddress || object.kind == okFieldAddress) {
      int sizeHint = scope.moduleScope->sizeHints.get(object.extraparam);
      if (sizeHint != 0) {
         ref_t wrapperRef = scope.moduleScope->typeHints.get(object.extraparam);         
         if (wrapperRef != 0)
            _writer.boxObject(*scope.tape, sizeHint, wrapperRef | mskVMTRef);
      }
   }

   return object;
}

ObjectInfo Compiler :: boxStructureField(CodeScope& scope, ObjectInfo object, int mode)
{
   int offset = object.param;

   int size = scope.moduleScope->sizeHints.get(object.extraparam);
   ref_t wrapper = scope.moduleScope->typeHints.get(object.extraparam);
   if (size == 2 || size == 4 || size == 8) {
      allocateStructure(scope, 0, object);

      _writer.loadBase(*scope.tape, object);
      _writer.loadObject(*scope.tape, ObjectInfo(okThisParam, 1));
      if (size == 2) {
         _writer.copyShort(*scope.tape, offset);
      }
      else _writer.copyInt(*scope.tape, offset);

   }
   else if (size == 8) {
      allocateStructure(scope, 0, object);

      _writer.loadBase(*scope.tape, object);
      _writer.loadObject(*scope.tape, ObjectInfo(okThisParam, 1));
      _writer.copyStructure(*scope.tape, offset, size);
      _writer.loadObject(*scope.tape, ObjectInfo(okBase));
   }
   else {
      // otherwise create a dynamic object and copy the content
      _writer.newStructure(*scope.tape, size, wrapper);
      _writer.loadBase(*scope.tape, ObjectInfo(okAccumulator));
      _writer.loadObject(*scope.tape, ObjectInfo(okThisParam, 1));
      _writer.copyStructure(*scope.tape, object.param, size);
      _writer.loadObject(*scope.tape, ObjectInfo(okBase));
   }

   return ObjectInfo(okAccumulator, 0, object.extraparam);
}

void Compiler :: compileMessageParameter(DNode& arg, CodeScope& scope, ref_t type_ref, int mode, size_t& count)
{
   if (arg == nsMessageParameter) {
      count++;

      ObjectInfo param = compileObject(arg.firstChild(), scope, mode & ~(HINT_DIRECT_ORDER | HINT_OUTEXPR));

      _writer.loadObject(*scope.tape, param);
       
      // if type is mismatch - typecast
      bool mismatch = false;
      compileTypecast(scope, param, type_ref, mismatch, HINT_TYPEENFORCING);
      if (mismatch)
         scope.raiseWarning(wrnTypeMismatch, arg.FirstTerminal());

      if (test(mode, HINT_DIRECT_ORDER)) {
         _writer.pushObject(*scope.tape, ObjectInfo(okAccumulator));
      }
      else _writer.saveObject(*scope.tape, ObjectInfo(okCurrent, count));

      arg = arg.nextNode();
   }
   else if (arg == nsTypedMessageParameter) {
      // if it is a typed message callback - directly call the message
      count++;

      ObjectInfo param = compileObject(arg.firstChild(), scope, mode & ~(HINT_DIRECT_ORDER | HINT_OUTEXPR));

      // if it is an output parameter - only locals / fields can be passed
      if (test(mode, HINT_OUTEXPR)) {
         if (param.kind != okLocal && param.kind != okField) {
            scope.raiseError(errInvalidOperation, arg.Terminal());
         }
      }

      _writer.loadObject(*scope.tape, param);
      bool mismatch = false;
      compileTypecast(scope, param, type_ref, mismatch, mode);

      if (test(mode, HINT_DIRECT_ORDER)) {
         _writer.pushObject(*scope.tape, ObjectInfo(okAccumulator));
      }
      else _writer.saveObject(*scope.tape, ObjectInfo(okCurrent, count));

      arg = arg.nextNode();
   }
}

ref_t Compiler :: mapMessage(DNode node, CodeScope& scope, ObjectInfo object, size_t& count, int& mode)
{
   bool             simpleParameters = true;

   TerminalInfo     verb = node.Terminal();
   IdentifierString signature;
   bool             first = true;

   ref_t            verb_id = _verbs.get(verb.value);
   int              paramCount = 0;
   DNode            arg = node.firstChild();

   // if it is a dispatch operation
   if (arg == nsTypedMessageParameter && verb_id != 0) {
      count = 1;

      mode |= HINT_GENERIC_MODE;

      if (arg.firstChild().firstChild() == nsNone)
         mode |= HINT_DIRECT_ORDER;

      return encodeMessage(0, verb_id, count);
   }

   if (verb_id == 0) {
      // provide namespace for the private message
      if (verb == tsPrivate) {
         signature.append(scope.moduleScope->project->StrSetting(opNamespace));
      }

      signature.append(verb);

      // if followed by argument list - it is EVAL verb
      if (arg != nsNone) {
         verb_id = EVAL_MESSAGE_ID;

         first = false;
      }
      // otherwise it is GET message
      else verb_id = GET_MESSAGE_ID;
   }

   // if message has generic argument list
   while (arg == nsMessageParameter) {
      count++;

      if (arg.firstChild().firstChild() != nsNone)
         simpleParameters = false;

      arg = arg.nextNode();
   }

   // if it is open argument list
   if (arg == nsMessageOpenParameter) {
      // if a generic argument is used with an open argument list
      // special postfix should be used
      if (count > 0) {
         if (!first)
            signature.append('&');

         signature.appendInt(count);
      }

      simpleParameters = false;

      arg = arg.firstChild();
      // check if argument list should be unboxed
      DNode param = arg.firstChild();

      if (arg.nextNode() == nsNone && param.firstChild() == nsNone && IsArgumentList(scope.mapObject(param.Terminal()))) {
         mode |= HINT_OARG_UNBOXING;
      }
      else {
         // terminator
         count += 1;

         while (arg != nsNone) {
            count++;

            arg = arg.nextNode();
         }
      }

      paramCount = OPEN_ARG_COUNT;
   }
   else {
      // if message has named argument list
      while (arg == nsSubjectArg) {
         TerminalInfo subject = arg.Terminal();

         if (!first) {
            signature.append('&');
         }
         else first = false;

         signature.append(subject);

         arg = arg.nextNode();

         // skip an argument
         if (arg == nsMessageParameter || arg == nsTypedMessageParameter) {
            count++;

            if (arg.firstChild().firstChild() != nsNone || arg == nsTypedMessageParameter)
               simpleParameters = false;

            arg = arg.nextNode();
         }
      }
      paramCount = count;

      if (paramCount >= OPEN_ARG_COUNT)
         scope.raiseError(errTooManyParameters, verb);

   }

   // if signature is presented
   ref_t sign_id = 0;
   if (!emptystr(signature)) {
      sign_id = scope.moduleScope->module->mapSubject(signature, false);
   }

   if (simpleParameters)
      mode |= HINT_DIRECT_ORDER;

   // create a message id
   return encodeMessage(sign_id, verb_id, paramCount);
}

void Compiler :: compileDirectMessageParameters(DNode arg, CodeScope& scope, int mode)
{
   // if it is a dispatch operation
   if (arg == nsTypedMessageParameter) {
      ObjectInfo param = compileObject(arg.firstChild(), scope, mode);
      // if it is open argument list
      if (param.kind == okParams) {
      }
      else _writer.pushObject(*scope.tape, param);
   }
   else if (arg == nsMessageParameter) {
      compileDirectMessageParameters(arg.nextNode(), scope, mode);

      ObjectInfo param = compileObject(arg.firstChild(), scope, mode);

      // box the object if required
      if (checkIfBoxingRequired(scope, param)) {
         _writer.loadObject(*scope.tape, param);

         boxObject(scope, param, mode);

         _writer.pushObject(*scope.tape, ObjectInfo(okAccumulator));
      }
      else _writer.pushObject(*scope.tape, param);
   }
   else if (arg == nsSubjectArg) {
      TerminalInfo subject = arg.Terminal();

      arg = arg.nextNode();

      // skip an argument without a parameter value
      while (arg == nsSubjectArg) {
         subject = arg.Terminal();

         arg = arg.nextNode();
      }

      bool out = false;
      ref_t type_ref = 0;
      int paramMode = HINT_DIRECT_ORDER;
      if (arg == nsTypedMessageParameter) {
         type_ref = scope.moduleScope->mapSubject(subject);
      }
      else type_ref = scope.moduleScope->mapType(subject, out);

      if (type_ref != 0) {
         paramMode |= HINT_TYPEENFORCING;

         if (out)
            paramMode |= HINT_OUTEXPR;
      }

      compileDirectMessageParameters(arg.nextNode(), scope, mode);

      size_t dummy = 0;
      compileMessageParameter(arg, scope, type_ref, paramMode, dummy);
   } 
}

void Compiler :: compilePresavedMessageParameters(DNode arg, CodeScope& scope, int mode, size_t& stackToFree)
{
   // if it is a dispatch operation
   if (arg == nsTypedMessageParameter) {
      _writer.loadObject(*scope.tape, compileObject(arg.firstChild(), scope, mode));
      _writer.saveObject(*scope.tape, ObjectInfo(okCurrent, 1));
   }
   else {
      size_t count = 0;

      // if message has generic argument list
      while (arg == nsMessageParameter) {
         count++;

         ObjectInfo param = compileObject(arg.firstChild(), scope, mode);
         _writer.loadObject(*scope.tape, param);

         // box the object if required
         boxObject(scope, param, mode);

         _writer.saveObject(*scope.tape, ObjectInfo(okCurrent, count));

         arg = arg.nextNode();
      }

      // if message has open argument list
      if (arg == nsMessageOpenParameter) {
         arg = arg.firstChild();

         stackToFree = 1;
         while (arg != nsNone) {
            count++;
            stackToFree++;

            ObjectInfo retVal = compileExpression(arg, scope, 0);
            _writer.loadObject(*scope.tape, retVal);

            // box object if required
            boxObject(scope, retVal, mode);

            _writer.saveObject(*scope.tape, ObjectInfo(okCurrent, count));

            arg = arg.nextNode();
         }
      }
      else {
         // if message has named argument list
         while (arg == nsSubjectArg) {
            TerminalInfo subject = arg.Terminal();

            arg = arg.nextNode();

            // skip an argument without a parameter value
            while (arg == nsSubjectArg) {
               subject = arg.Terminal();

               arg = arg.nextNode();
            }

            bool out = false;
            ref_t type_ref = 0;
            int paramMode = 0;
            if (arg == nsTypedMessageParameter) {
               type_ref = scope.moduleScope->mapSubject(subject);
            }
            else type_ref = scope.moduleScope->mapType(subject, out);

            if (type_ref != 0) {
               paramMode |= HINT_TYPEENFORCING;

               if (out)
                  paramMode |= HINT_OUTEXPR;
            }

            compileMessageParameter(arg, scope, type_ref, paramMode, count);
         }
      }
   }
}

void Compiler :: compileUnboxedMessageParameters(DNode node, CodeScope& scope, int mode, int count, size_t& stackToFree)
{
   // unbox the argument list
   DNode arg = goToSymbol(node, nsMessageOpenParameter).firstChild();
   ObjectInfo list = compileObject(arg.firstChild(), scope, mode);
   _writer.loadObject(*scope.tape, list);

   // unbox the argument list and return the object saved before the list
   _writer.unboxArgList(*scope.tape);

   // reserve the place for target and generic message parameters if available and save the target
   _writer.declareArgumentList(*scope.tape, count + 1);
   _writer.saveObject(*scope.tape, ObjectInfo(okCurrent));

   count = 0;
   // if message has generic argument list
   arg = node;
   while (arg == nsMessageParameter) {
      count++;

      ObjectInfo param = compileObject(arg.firstChild(), scope, mode);
      _writer.loadObject(*scope.tape, param);

      // box the object if required
      boxObject(scope, param, mode);

      _writer.saveObject(*scope.tape, ObjectInfo(okCurrent, count));

      arg = arg.nextNode();
   }

   // indicate that the stack to be freed cannot be defined at compile-time
   stackToFree = (size_t)-1;
}

ref_t Compiler :: compileMessageParameters(DNode node, CodeScope& scope, ObjectInfo object, int& mode, size_t& spaceToRelease)
{
   size_t count = 0;
   ref_t messageRef = mapMessage(node, scope, object, count, mode);

   // if the target is in register(i.e. it is the result of the previous operation), direct message compilation is not possible
   if (object.kind == okAccumulator) {
      mode &= ~HINT_DIRECT_ORDER;
   }
   else if (count == 1) {
      mode |= HINT_DIRECT_ORDER;
   }

   // if only simple arguments are used we could directly save parameters
   if (test(mode, HINT_DIRECT_ORDER)) {
      compileDirectMessageParameters(node.firstChild(), scope, mode);

      _writer.loadObject(*scope.tape, object);
      _writer.pushObject(*scope.tape, ObjectInfo(okAccumulator));
   }
   // if open argument list should be unboxed
   else if (test(mode, HINT_OARG_UNBOXING)) {
      // save the target
      _writer.loadObject(*scope.tape, object);

      // box object if required
      object = boxObject(scope, object, mode);

      _writer.pushObject(*scope.tape, ObjectInfo(okAccumulator));

      compileUnboxedMessageParameters(node.firstChild(), scope, mode & ~HINT_OARG_UNBOXING, count, spaceToRelease);
   }
   // otherwise the space should be allocated first,
   // to garantee the correct order of parameter evaluation
   else {
      _writer.declareArgumentList(*scope.tape, count + 1);
      _writer.loadObject(*scope.tape, object);
      _writer.saveObject(*scope.tape, ObjectInfo(okCurrent));

      compilePresavedMessageParameters(node.firstChild(), scope, mode, spaceToRelease);
   }

   return messageRef;
}

ObjectInfo Compiler :: compileBranchingOperator(DNode& node, CodeScope& scope, ObjectInfo object, int mode, int operator_id)
{
   _writer.loadObject(*scope.tape, object);

   DNode elsePart = node.select(nsElseOperation);
   if (elsePart != nsNone) {
      _writer.declareThenElseBlock(*scope.tape);
      compileBranching(node, scope, object, operator_id, HINT_SUBBRANCH);
      _writer.declareElseBlock(*scope.tape);
      compileBranching(elsePart, scope, object, 0, 0); // for optimization, the condition is checked only once
      _writer.endThenBlock(*scope.tape);
   }
   else if (test(mode, HINT_LOOP)) {
      compileBranching(node, scope, object, operator_id, HINT_SUBBRANCH);
      _writer.jump(*scope.tape, true);
   }
   else {
      _writer.declareThenBlock(*scope.tape);
      compileBranching(node, scope, object, operator_id, 0);
      _writer.endThenBlock(*scope.tape);
   }

   return ObjectInfo(okAccumulator, 0);
}

ref_t Compiler :: mapConstantType(ModuleScope& scope, ObjectKind kind)
{
   if (kind == okIntConstant) {
      return scope.getIntType();
   }
   else if (kind == okLongConstant) {
      return scope.getLongType();
   }
   else if (kind == okRealConstant) {
      return scope.getRealType();
   }
   else if (kind == okLiteralConstant) {
      return scope.getLiteralType();
   }
   else return 0;
}

bool Compiler :: compileInlineArithmeticOperator(CodeScope& scope, int operator_id, ObjectInfo loperand, ObjectInfo roperand, ObjectInfo& result, int mode)
{
   int intType = scope.moduleScope->getIntType();
   int longType = scope.moduleScope->getLongType();
   int realType = scope.moduleScope->getRealType();
   bool assignMode = test(mode, HINT_ASSIGN_MODE);

   // check
   if (loperand.extraparam == intType && roperand.extraparam == intType) {
      result.extraparam = intType;
   }
   else if (loperand.extraparam == longType && roperand.extraparam == longType) {
      result.extraparam = longType;
   }
   else if (operator_id == AND_MESSAGE_ID || operator_id == OR_MESSAGE_ID || operator_id == XOR_MESSAGE_ID) {
      return false;
   }
   else if (loperand.extraparam == realType && roperand.extraparam == realType) {
      result.extraparam = realType;
   }
   else return false;

   if (assignMode) {
      _writer.popObject(*scope.tape, ObjectInfo(okBase));
      _writer.popObject(*scope.tape, ObjectInfo(okAccumulator));

      result.kind = okIdle;
   }
   else {
      allocateStructure(scope, 0, result);
      _writer.loadBase(*scope.tape, result);
      _writer.popObject(*scope.tape, ObjectInfo(okAccumulator));

      if (result.extraparam == intType) {
         _writer.assignInt(*scope.tape, ObjectInfo(okBase));
      }
      else _writer.assignLong(*scope.tape, ObjectInfo(okBase));

      _writer.popObject(*scope.tape, ObjectInfo(okAccumulator));
   }

   if (result.extraparam == intType) {
      _writer.doIntOperation(*scope.tape, operator_id);
   }
   else if (result.extraparam == longType) {
      _writer.doLongOperation(*scope.tape, operator_id);
   }
   else if (result.extraparam == realType) {
      _writer.doRealOperation(*scope.tape, operator_id);
   }

   _writer.loadObject(*scope.tape, ObjectInfo(okBase));

   return true;
}

bool Compiler :: compileInlineComparisionOperator(CodeScope& scope, int operator_id, ObjectInfo loperand, ObjectInfo roperand, ObjectInfo& result, bool& invertMode)
{
   int intType = scope.moduleScope->getIntType();
   int longType = scope.moduleScope->getLongType();
   int realType = scope.moduleScope->getRealType();
   int literalType = scope.moduleScope->getLiteralType();

   // check
   if (loperand.extraparam == intType && roperand.extraparam == intType) {
   }
   else if (loperand.extraparam == longType && roperand.extraparam == longType) {
   }
   else if (loperand.extraparam == realType && roperand.extraparam == realType) {
   }
   else if (loperand.extraparam == literalType && roperand.extraparam == literalType) {
   }
   else return false;

   if (operator_id == GREATER_MESSAGE_ID) {
      _writer.popObject(*scope.tape, ObjectInfo(okBase));
      _writer.popObject(*scope.tape, ObjectInfo(okAccumulator));

      operator_id = LESS_MESSAGE_ID;
   }
   else {
      _writer.popObject(*scope.tape, ObjectInfo(okAccumulator));
      _writer.popObject(*scope.tape, ObjectInfo(okBase));
   }

   if (loperand.extraparam == intType) {
      _writer.doIntOperation(*scope.tape, operator_id);
   }
   else if (loperand.extraparam == longType) {
      _writer.doLongOperation(*scope.tape, operator_id);
   }
   else if (loperand.extraparam == realType) {
      _writer.doRealOperation(*scope.tape, operator_id);
   }
   else if (loperand.extraparam == literalType) {
      _writer.doLiteralOperation(*scope.tape, operator_id);
   }

   if (invertMode) {
       _writer.selectConstant(*scope.tape, scope.moduleScope->trueReference, scope.moduleScope->falseReference);

       invertMode = false;
   }
   else _writer.selectConstant(*scope.tape, scope.moduleScope->falseReference, scope.moduleScope->trueReference);

   result.extraparam = scope.moduleScope->getBoolType();

   return true;
}

bool Compiler :: compileInlineReferOperator(CodeScope& scope, int operator_id, ObjectInfo loperand, ObjectInfo roperand, ObjectInfo& result)
{
   int paramsType = scope.moduleScope->getParamsType();
   int intType = scope.moduleScope->getIntType();

   if (loperand.extraparam == paramsType && roperand.extraparam == intType) {
   }
   else return false;

   _writer.popObject(*scope.tape, ObjectInfo(okBase));
   _writer.popObject(*scope.tape, ObjectInfo(okAccumulator));

   _writer.doArrayOperation(*scope.tape, operator_id);

   return true;
}

ObjectInfo Compiler :: compileOperator(DNode& node, CodeScope& scope, ObjectInfo object, int mode)
{
   ObjectInfo retVal(okAccumulator);

   TerminalInfo operator_name = node.Terminal();
   int operator_id = _operators.get(operator_name);

   // if it is branching operators
   if (operator_id == IF_MESSAGE_ID || operator_id == IFNOT_MESSAGE_ID) {
      return compileBranchingOperator(node, scope, object, mode, operator_id);
   }

   // HOTFIX : recognize SET_REFER_MESSAGE_ID
   if (operator_id == REFER_MESSAGE_ID && node.nextNode() == nsAssigning)
      operator_id = SET_REFER_MESSAGE_ID;

   bool dblOperator = IsDoubleOperator(operator_id);
   bool notOperator = IsInvertedOperator(operator_id);

   ObjectInfo operand;
   ObjectInfo operand2;

   // if the operation parameters can be compiled directly
   if (!dblOperator && object.kind != okAccumulator) {
      operand = compileExpression(node, scope, 0);

      _writer.pushObject(*scope.tape, operand);
      _writer.pushObject(*scope.tape, object);
   }
   else {
      _writer.declareArgumentList(*scope.tape, dblOperator ? 3 : 2);

      _writer.loadObject(*scope.tape, object);
      _writer.saveObject(*scope.tape, ObjectInfo(okCurrent, 0));

      if (dblOperator) {
         operand2 = compileExpression(node.nextNode().firstChild(), scope, 0);

         _writer.pushObject(*scope.tape, operand2);
      }

      operand = compileExpression(node, scope, 0);
      _writer.loadObject(*scope.tape, operand);
      _writer.saveObject(*scope.tape, ObjectInfo(okCurrent, 1));
   }

   recordStep(scope, node.Terminal(), dsProcedureStep);

   // map constant types
   if (object.extraparam == 0) {
      object.extraparam = mapConstantType(*scope.moduleScope, object.kind);
   }
   if (operand.extraparam == 0) {
      operand.extraparam = mapConstantType(*scope.moduleScope, operand.kind);
   }

   if (IsExprOperator(operator_id) && compileInlineArithmeticOperator(scope, operator_id, object, operand, retVal, mode)) {
      // if inline arithmetic operation is implemented
      // do nothing
   }
   else if (IsCompOperator(operator_id) && compileInlineComparisionOperator(scope, operator_id, object, operand, retVal, notOperator)) {
      // if inline comparision operation is implemented
      // do nothing
   }
   else if (IsReferOperator(operator_id) && compileInlineReferOperator(scope, operator_id, object, operand, retVal)) {
      // if inline referring operation is implemented
      // do nothing
   }
   else {
      // box operands if necessary
      if (checkIfBoxingRequired(scope, object)) {
         _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 0));
         object = boxObject(scope, object, mode);
         _writer.saveObject(*scope.tape, ObjectInfo(okCurrent, 0));
      }
      if (checkIfBoxingRequired(scope, operand)) {
         _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 1));
         operand = boxObject(scope, operand, mode);
         _writer.saveObject(*scope.tape, ObjectInfo(okCurrent, 1));
      }
      if (operand2.kind != okUnknown && checkIfBoxingRequired(scope, operand2)) {
         _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 2));
         operand2 = boxObject(scope, operand2, mode);
         _writer.saveObject(*scope.tape, ObjectInfo(okCurrent, 2));
      }

      int message_id = encodeMessage(0, operator_id, dblOperator ? 2 : 1);

      compileMessage(node, scope, object, message_id, HINT_GENERIC_MODE);
   }

   if (notOperator) {
      ModuleScope* moduleScope = scope.moduleScope;

      bool mismatch = false;
      compileTypecast(scope, retVal, scope.moduleScope->getBoolType(), mismatch, HINT_TYPEENFORCING);

      _writer.invertBool(*scope.tape, moduleScope->trueReference, moduleScope->falseReference);
   }

   return retVal;
}

ObjectInfo Compiler :: compileMessage(DNode node, CodeScope& scope, ObjectInfo object, int messageRef, int mode)
{
   ObjectInfo retVal(okAccumulator);

   if (messageRef == 0)
      scope.raiseError(errUnknownMessage, node.Terminal());

   int signRef = getSignature(messageRef);
   int paramCount = getParamCount(messageRef);
   bool catchMode = test(mode, HINT_CATCH);

   // if it is generic dispatch (NOTE: param count is set to zero as the marker)
   if (test(mode, HINT_GENERIC_MODE)) {
      _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 0));
      _writer.setMessage(*scope.tape, messageRef);
      _writer.callMethod(*scope.tape, 1, 1);
   }
   // otherwise compile message call
   else {
      _writer.setMessage(*scope.tape, messageRef);

   //   bool directMode = test(_optFlag, optDirectConstant);

      recordStep(scope, node.Terminal(), dsProcedureStep);

      // if static message is sent to a class class
      if (object.kind == okConstantClass) {
         _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 0));
         _writer.callResolvedMethod(*scope.tape, object.extraparam, messageRef);

         // try to define the type if any
         retVal.extraparam = scope.moduleScope->getClassType(object.param);
      }
      // if static message is sent to a integer constant class
      else if (object.kind == okIntConstant) {
         _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 0));
         _writer.callResolvedMethod(*scope.tape, scope.moduleScope->mapConstantReference(INT_CLASS), messageRef);
      }
      // if static message is sent to a long integer constant class
      else if (object.kind == okLongConstant) {
         _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 0));
         _writer.callResolvedMethod(*scope.tape, scope.moduleScope->mapConstantReference(LONG_CLASS), messageRef);
      }
      // if static message is sent to a integer constant class
      else if (object.kind == okRealConstant) {
         _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 0));
         _writer.callResolvedMethod(*scope.tape, scope.moduleScope->mapConstantReference(REAL_CLASS), messageRef);
      }
      // if static message is sent to a integer constant class
      else if (object.kind == okLiteralConstant) {
         _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 0));
         _writer.callResolvedMethod(*scope.tape, scope.moduleScope->mapConstantReference(WSTR_CLASS), messageRef);
      }
      // if external role is provided
      else if (object.kind == okConstantRole) {
         _writer.callResolvedMethod(*scope.tape, object.param, messageRef);
      }
      else if (object.kind == okConstantSymbol) {
         _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 0));
         _writer.callResolvedMethod(*scope.tape, object.param, messageRef);
      }
      // if message sent to the class parent
      else if (object.kind == okSuper) {
         _writer.loadObject(*scope.tape, ObjectInfo(okThisParam, 1));
         _writer.callResolvedMethod(*scope.tape, object.param, messageRef);
      }
      // if message sent to the $self
      else if (object.kind == okThisParam && test(scope.getClassFlags(false), elSealed)) {
         _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 0));
         _writer.callResolvedMethod(*scope.tape, scope.getClassRefId(false), messageRef);
      }
      // if run-time external role is provided
      else if (object.kind == okRole) {
         _writer.callRoleMessage(*scope.tape, paramCount);
      }
      else if (catchMode) {
         _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 0));
         _writer.callMethod(*scope.tape, 0, paramCount);
      }
      else {
         _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 0));

         ref_t wrapper = object.extraparam != 0 ? scope.moduleScope->typeHints.get(object.extraparam) : 0;
         // check if it is a type wrapper
         if (wrapper) {
            // check if the message is supported
            if (scope.moduleScope->checkTypeMethod(object.extraparam, messageRef)) {
               _writer.callResolvedMethod(*scope.tape, wrapper, messageRef);
            }
            else {
               scope.raiseWarning(wrnUnknownMessage, node.FirstTerminal());
               _writer.loadObject(*scope.tape, ObjectInfo(okConstantSymbol, scope.moduleScope->mapConstantReference(NOMETHOD_EXCEPTION_CLASS)));
               _writer.throwCurrent(*scope.tape);
            }
         }
         else _writer.callMethod(*scope.tape, 0, paramCount);
      }
   }

   // the result of get&type message should be typed
   if (paramCount == 0 && getVerb(messageRef) == GET_MESSAGE_ID) {
      if (scope.moduleScope->typeHints.exist(signRef)) {
         return ObjectInfo(okAccumulator, 0, signRef);
      }
      else return ObjectInfo(okAccumulator, 0, scope.moduleScope->synonymHints.get(signRef));
   }
   else return retVal;
}

ObjectInfo Compiler :: compileMessage(DNode node, CodeScope& scope, ObjectInfo object, int mode)
{
   size_t spaceToRelease = 0;
   ref_t messageRef = compileMessageParameters(node, scope, object, mode, spaceToRelease);

   // use dynamic extension if exists
   if (scope.moduleScope->extensionHints.exist(messageRef, getType(object))) {
      SubjectMap* typeExtensions = scope.moduleScope->extensions.get(getType(object));

      ref_t roleRef = typeExtensions->get(messageRef);
      if (roleRef != 0)
         object = ObjectInfo(okConstantRole, roleRef);
   }

   if (spaceToRelease > 0) {
      // if open argument list is used
      ObjectInfo retVal = compileMessage(node, scope, object, messageRef, mode);

      // it should be removed from the stack
      if (spaceToRelease == (size_t)-1) {
         _writer.releaseArgList(*scope.tape);
         _writer.releaseObject(*scope.tape);
      }
      else _writer.releaseObject(*scope.tape, spaceToRelease);

      return retVal;
   }
   else return compileMessage(node, scope, object, messageRef, mode);
}

ObjectInfo Compiler :: compileEvalMessage(DNode& node, CodeScope& scope, ObjectInfo object, int mode)
{
   size_t count = countSymbol(node, nsMessageParameter);

   _writer.declareArgumentList(*scope.tape, count + 1);
   _writer.loadObject(*scope.tape, object);
   
   boxObject(scope, object, mode);

   _writer.saveObject(*scope.tape, ObjectInfo(okCurrent));

   size_t stackToFree = 0;
   compilePresavedMessageParameters(node, scope, mode, stackToFree);

   // skip all except the last message parameter
   while (node.nextNode() == nsMessageParameter)
      node = node.nextNode();

   return compileMessage(node, scope, object, encodeMessage(0, EVAL_MESSAGE_ID, count), mode);
}

ObjectInfo Compiler :: compileOperations(DNode node, CodeScope& scope, ObjectInfo object, int mode)
{
   ObjectInfo currentObject = object;

   DNode member = node.nextNode();

   if (object.kind == okExternal) {
      currentObject = compileExternalCall(member, scope, node.Terminal(), mode);
      if (test(mode, HINT_TRY)) {
         // skip error handling for the external operation
         mode &= ~HINT_TRY;

         member = member.nextNode();
      }
      member = member.nextNode();
   }

   bool catchMode = false;
   if (test(mode, HINT_TRY)) {
      _writer.declareTry(*scope.tape);

      mode &= ~HINT_TRY;
   }

   while (member != nsNone) {
      if (member == nsExtension) {
         currentObject = compileExtension(member, scope, currentObject, mode);
      }
      else if (member==nsMessageOperation) {
         currentObject = compileMessage(member, scope, currentObject, mode);
      }
      else if (member==nsMessageParameter) {
         currentObject = compileEvalMessage(member, scope, currentObject, mode);
      }
      else if (member == nsSwitching) {
         compileSwitch(member, scope, currentObject);

         currentObject = ObjectInfo(okAccumulator);
      }
      else if (member == nsAssigning) {
         // if primitive data operation can be used
         if (object.extraparam != 0 && scope.moduleScope->sizeHints.exist(object.extraparam)) {
            int assignMode = 0;
            if (object.kind == okLocal || object.kind == okFieldAddress) {
               // if it is an assignment operation (e.g. a := a + b <=> a += b)
               if (isAssignOperation(node, member)) {
                  assignMode |= HINT_ASSIGN_MODE;
               }
            }

            ObjectInfo info = compileExpression(member.firstChild(), scope, assignMode);
            if (info.kind == okIndexAccumulator) {
               // if it is a primitive operation
               compileContentAssignment(member, scope, currentObject, info);
            }
            else if (info.kind == okIdle) {
               // if assigning was already done - do nothing
            }
            else {
               _writer.loadObject(*scope.tape, info);

               // HOTFIX: if it is a "quasi" typed field (a typed field implemented as a normal one) - assign as an object
               if (currentObject.kind == okField) {
                  if (checkIfBoxingRequired(scope, info))
                     info = boxObject(scope, info, mode);

                  bool mismatch = false;
                  compileTypecast(scope, info, object.extraparam, mismatch, HINT_TYPEENFORCING);
                  if (mismatch)
                     scope.raiseWarning(wrnTypeMismatch, node.Terminal());

                  compileAssignment(member, scope, currentObject);
               }
               // otherwise assign data content
               else compileContentAssignment(member, scope, currentObject, info);
            }
         }
         else {
            ObjectInfo info = compileExpression(member.firstChild(), scope, 0);

            _writer.loadObject(*scope.tape, info);

            if (checkIfBoxingRequired(scope, info))
               info = boxObject(scope, info, mode);

            bool mismatch = false;
            compileTypecast(scope, info, object.extraparam, mismatch, HINT_TYPEENFORCING);
            if (mismatch)
               scope.raiseWarning(wrnTypeMismatch, node.Terminal());

            compileAssignment(member, scope, currentObject);
         }

         currentObject = ObjectInfo(okAccumulator);
      }
      else if (member == nsAltMessageOperation) {
         if (!catchMode) {
            _writer.declareCatch(*scope.tape);
            catchMode = true;
         }
         currentObject = compileMessage(member, scope, ObjectInfo(okAccumulator), mode | HINT_CATCH);
      }
      else if (member == nsL3Operation || member == nsL4Operation || member == nsL5Operation || member == nsL6Operation
         || member == nsL7Operation || member == nsL0Operation)
      {
         currentObject = compileOperator(member, scope, currentObject, mode);
      }
      member = member.nextNode();
   }

   if (catchMode) {
      _writer.endCatch(*scope.tape);
   }

   return currentObject;
}

ObjectInfo Compiler :: compileExtension(DNode& node, CodeScope& scope, ObjectInfo object, int mode)
{
   ModuleScope* moduleScope = scope.moduleScope;
   ObjectInfo   role;

   DNode roleNode = node.firstChild();
   // check if the extension can be used as a static role (it is constant)
   if (roleNode.firstChild() == nsNone) {
      int flags = 0;

      role = scope.mapObject(roleNode.Terminal());
      if (role.kind == okSymbol || role.kind == okConstantSymbol) {
         // if the symbol is used inside itself
         if (role.param == scope.getClassRefId()) {
            flags = scope.getClassFlags();
         }
         // otherwise
         else {
            ClassInfo roleClass;
            moduleScope->loadClassInfo(roleClass, moduleScope->module->resolveReference(role.param));

            flags = roleClass.header.flags;
         }
      }
      // if the symbol VMT can be used as an external role
      if (test(flags, elStateless)) {
         role = ObjectInfo(okConstantRole, role.param);
      }
      else role = ObjectInfo(okRole);
   }
   else role = ObjectInfo(okRole);

   // override standard message compiling routine
   node = node.nextNode();

   size_t spaceToRelease = 0;
   int dummy = 0;
   ref_t messageRef = compileMessageParameters(node, scope, object, dummy, spaceToRelease);

   ObjectInfo retVal(okAccumulator);
   // if it is dispatching
   if (node.firstChild() == nsTypedMessageParameter && getParamCount(messageRef) == 0) {
      _writer.loadObject(*scope.tape, ObjectInfo(okCurrent, 1));
      // HOTFIX : include the parameter
      _writer.setMessage(*scope.tape, messageRef + 1);
      _writer.callMethod(*scope.tape, 1, 1);
   }
   else {
      // if it is a not a constant, compile a role
      if (role.kind != okConstantSymbol)
         _writer.loadObject(*scope.tape, compileObject(roleNode, scope, mode));

      // send message to the role
      retVal = compileMessage(node, scope, role, messageRef, mode);
   }

   if (spaceToRelease > 0)
      _writer.releaseObject(*scope.tape, spaceToRelease);

   return retVal;
}

void Compiler :: compileActionVMT(DNode node, InlineClassScope& scope, DNode argNode)
{
   _writer.declareClass(scope.tape, scope.reference);

   ActionScope methodScope(&scope);
   methodScope.message = encodeVerb(EVAL_MESSAGE_ID);

   if (argNode != nsNone) {
      // define message parameter
      methodScope.message = declareInlineArgumentList(argNode, methodScope);

      node = node.select(nsSubCode);
   }

   // if it is single expression
   DNode expr = node.firstChild();
   if (isExpressionAction(expr)) {
      compileExpressionAction(expr, methodScope);
   }
   else compileAction(node, methodScope);

   _writer.endClass(scope.tape);

   // stateless inline class
   if (scope.info.fields.Count()==0 && !test(scope.info.header.flags, elStructureRole)) {
      scope.info.header.flags |= elStateless;
   }
   else scope.info.header.flags &= ~elStateless;

   // optimize
   optimizeTape(scope.tape);

   // create byte code sections
   scope.save(_writer);
}

void Compiler :: compileNestedVMT(DNode node, InlineClassScope& scope)
{
   _writer.declareClass(scope.tape, scope.reference);

   DNode member = node.firstChild();
   compileVMT(member, scope);

   _writer.endClass(scope.tape);

   // stateless inline class
   if (scope.info.fields.Count()==0 && !test(scope.info.header.flags, elStructureRole)) {
      scope.info.header.flags |= elStateless;
   }
   else scope.info.header.flags &= ~elStateless;

   // optimize
   optimizeTape(scope.tape);

   // create byte code sections
   scope.save(_writer);
}

ObjectInfo Compiler :: compileNestedExpression(DNode node, CodeScope& ownerScope, InlineClassScope& scope, int mode)
{
   if (test(scope.info.header.flags, elStateless)) {
      // if it is a stateless class

      return ObjectInfo(okConstantSymbol, scope.reference);
   }
   else {
      int presaved = 0;

      // unbox all typed variables
      Map<const wchar16_t*, InlineClassScope::Outer>::Iterator outer_it = scope.outers.start();
      while(!outer_it.Eof()) {
         ObjectInfo info = (*outer_it).outerObject;

         if (checkIfBoxingRequired(ownerScope, info)) {
            _writer.loadObject(*ownerScope.tape, info);
            boxObject(ownerScope, info, 0);
            _writer.pushObject(*ownerScope.tape, ObjectInfo(okAccumulator));
            presaved++;
         }

         outer_it++;
      }

      // dynamic binary symbol
      if (test(scope.info.header.flags, elStructureRole)) {
         _writer.newStructure(*ownerScope.tape, scope.info.size, scope.reference);

         if (scope.outers.Count() > 0)
            scope.raiseError(errInvalidInlineClass, node.Terminal());
      }
      // dynamic normal symbol
      else _writer.newObject(*ownerScope.tape, scope.info.fields.Count(), scope.reference);

      _writer.loadBase(*ownerScope.tape, ObjectInfo(okAccumulator));

      outer_it = scope.outers.start();
      int toFree = 0;
      while(!outer_it.Eof()) {
         ObjectInfo info = (*outer_it).outerObject;

         //NOTE: info should be either fields or locals
         if (info.kind == okOuterField) {
            _writer.loadObject(*ownerScope.tape, info);
            _writer.saveBase(*ownerScope.tape, info, (*outer_it).reference);
         }
         else if (info.kind == okParam || info.kind == okLocal || info.kind == okField || info.kind == okFieldAddress) {
            if (checkIfBoxingRequired(ownerScope, info)) {
               _writer.saveBase(*ownerScope.tape, ObjectInfo(okCurrent, --presaved), (*outer_it).reference);
               toFree++;
            }
            else _writer.saveBase(*ownerScope.tape, info, (*outer_it).reference);
         }
         else _writer.saveBase(*ownerScope.tape, info, (*outer_it).reference);

         outer_it++;
      }

      _writer.releaseObject(*ownerScope.tape, toFree);
      _writer.loadObject(*ownerScope.tape, ObjectInfo(okBase));

      return ObjectInfo(okAccumulator);
   }
}

ObjectInfo Compiler :: compileNestedExpression(DNode node, CodeScope& ownerScope, int mode)
{
//   recordStep(ownerScope, node.Terminal(), dsStep);

   ref_t typeRef = 0;
   InlineClassScope scope(&ownerScope, mapNestedExpression(ownerScope, mode, typeRef));

   // nested class is sealed
   scope.info.header.flags |= elSealed;

   // if it is an action code block
   if (node == nsSubCode) {
      if (isExpressionAction(node.firstChild())) {
         compileParentDeclaration(scope.moduleScope->mapConstantReference(EXPRESSION_CLASS), scope);
      }
      else compileParentDeclaration(scope.moduleScope->mapConstantReference(ACTION_CLASS), scope);

      compileActionVMT(node, scope, DNode());
   }
   // if it is an action code block
   else if (node == nsMethodParameter || node == nsSubjectArg) {
      compileParentDeclaration(scope.moduleScope->mapConstantReference(ACTION_CLASS), scope);

      compileActionVMT(goToSymbol(node, nsInlineExpression), scope, node);
   }
   // if it is a shortcut action code block 
   else if (node == nsObject) {
      compileParentDeclaration(scope.moduleScope->mapConstantReference(ACTION_CLASS), scope);

      compileActionVMT(node.firstChild(), scope, node);
   }
   // if it is inherited nested class
   else if (node.Terminal() != nsNone) {
	   // inherit parent
      compileParentDeclaration(node, scope);

      compileNestedVMT(node.firstChild(), scope);
   }
   // if it is normal nested class
   else {
      compileParentDeclaration(DNode(), scope);

      // define dispatch type if provided
      if (typeRef)
         scope.info.header.typeRef = typeRef;

      compileNestedVMT(node, scope);
   }
   return compileNestedExpression(node, ownerScope, scope, mode & ~HINT_ROOT);
}

ObjectInfo Compiler :: compileCollection(DNode objectNode, CodeScope& scope, int mode)
{
   return compileCollection(objectNode, scope, mode, scope.moduleScope->mapConstantReference(ARRAY_CLASS));
}

ObjectInfo Compiler :: compileCollection(DNode node, CodeScope& scope, int mode, ref_t vmtReference)
{
   int counter = 0;

   // all collection memebers should be created before the collection itself
   while (node != nsNone) {
      saveObject(scope, compileExpression(node, scope, mode), mode);

      node = node.nextNode();
      counter++;
   }

   // create the collection
   _writer.newObject(*scope.tape, counter, vmtReference);

   _writer.loadBase(*scope.tape, ObjectInfo(okAccumulator));

   // assign the members
   while (counter > 0) {
      _writer.popObject(*scope.tape, ObjectInfo(okAccumulator));
      _writer.saveBase(*scope.tape, ObjectInfo(okAccumulator), counter - 1);

      counter--;
   }

   _writer.loadObject(*scope.tape, ObjectInfo(okBase));

   return ObjectInfo(okAccumulator);
}

ObjectInfo Compiler :: compileGetProperty(DNode node, CodeScope& scope, int mode, ref_t vmtReference)
{
   // compile property subject
   saveObject(scope, compileMessageReference(node, scope, mode), mode);

   // compile property content
   saveObject(scope, compileExpression(goToSymbol(node.firstChild(), nsExpression), scope, mode), mode);

   // create the collection
   _writer.newObject(*scope.tape, 2, vmtReference);
   _writer.loadBase(*scope.tape, ObjectInfo(okAccumulator));

   // assign the members
   _writer.saveBase(*scope.tape, ObjectInfo(okCurrent), 1);
   _writer.saveBase(*scope.tape, ObjectInfo(okCurrent, 1), 0);

   _writer.loadObject(*scope.tape, ObjectInfo(okBase));
   _writer.releaseObject(*scope.tape, 2);

   return ObjectInfo(okAccumulator);
}

ObjectInfo Compiler :: compileGetProperty(DNode objectNode, CodeScope& scope, int mode)
{
   return compileGetProperty(objectNode, scope, mode, scope.moduleScope->mapConstantReference(GETPROPERTY_CLASS));
}

ObjectInfo Compiler :: compileTypecast(CodeScope& scope, ObjectInfo target, ref_t type_ref, bool& mismatch, int mode)
{
   if (type_ref != 0) {
      ModuleScope* moduleScope = scope.moduleScope;

      if (test(mode, HINT_TYPEENFORCING) && moduleScope->typeHints.exist(type_ref)) {
         // typecast numeric constant
         if (target.kind == okIntConstant) {
            // if type is compatible with the integer value
            int size = moduleScope->sizeHints.get(type_ref);
            if (size > 0 && size <= 4) {
               return ObjectInfo(okAccumulator, 0, type_ref);
            }
         }
         else if (target.kind == okLongConstant && type_ref == moduleScope->getLongType()) {
            return ObjectInfo(okAccumulator, 0, type_ref);
         }
         else if (target.kind == okRealConstant && type_ref == moduleScope->getRealType()) {
            return ObjectInfo(okAccumulator, 0, type_ref);
         }
         else if (target.kind == okLiteralConstant && type_ref == moduleScope->getLiteralType()) {
            return ObjectInfo(okAccumulator, 0, type_ref);
         }
         if (target.extraparam != type_ref) {
            // if type mismatch
            mismatch = true;

            // if they are not synonym
            // call typecast method
            boxObject(scope, target, 0);

            _writer.setMessage(*scope.tape, encodeMessage(type_ref, 0, 0));
            _writer.typecast(*scope.tape);

            return ObjectInfo(okAccumulator, 0, type_ref);
         }
      }
      else {
         _writer.pushObject(*scope.tape, ObjectInfo(okAccumulator));
         _writer.setMessage(*scope.tape, encodeMessage(type_ref, GET_MESSAGE_ID, 0));
         _writer.callMethod(*scope.tape, 0, 0);

         return ObjectInfo(okAccumulator);
      }
   }
   return target;
}

ObjectInfo Compiler :: compileRetExpression(DNode node, CodeScope& scope, int mode)
{
   ObjectInfo info = compileExpression(node, scope, mode);

   _writer.loadObject(*scope.tape, info);

   // box object if required
   boxObject(scope, info, mode);

   // type cast object if required
   int verb, paramCount;
   ref_t subj;
   decodeMessage(scope.getMessageID(), subj, verb, paramCount);
   if (verb == GET_MESSAGE_ID && paramCount == 0) {
      if (scope.moduleScope->synonymHints.exist(subj)) {
         subj = scope.moduleScope->synonymHints.get(subj);
      }
      if (scope.moduleScope->typeHints.exist(subj)) {
         bool mismatch = false;
         compileTypecast(scope, info, subj, mismatch, HINT_TYPEENFORCING);
         if (mismatch)
            scope.raiseWarning(wrnTypeMismatch, node.FirstTerminal());
      }
   }

   scope.freeSpace();

   _writer.declareBreakpoint(*scope.tape, 0, 0, 0, dsVirtualEnd);

   return ObjectInfo(okAccumulator);
}

ObjectInfo Compiler :: compileExpression(DNode node, CodeScope& scope, int mode)
{
   DNode member = node.firstChild();

   ObjectInfo objectInfo;
   if (member==nsObject) {
      objectInfo = compileObject(member, scope, mode & ~HINT_OUTEXPR);
   }
   if (member != nsNone) {
      if (findSymbol(member, nsAltMessageOperation)) {
         objectInfo = compileOperations(member, scope, objectInfo, (mode | HINT_TRY) & ~HINT_ROOT);
      }
      else objectInfo = compileOperations(member, scope, objectInfo, mode & ~HINT_ROOT);
   }

   return objectInfo;
}

ObjectInfo Compiler :: compileBranching(DNode thenNode, CodeScope& scope, ObjectInfo target, int verb, int subCodeMode)
{
   // execute the block if the condition
   // is true / false
   if (verb == IF_MESSAGE_ID || verb == IFNOT_MESSAGE_ID) {
      _writer.declareBreakpoint(*scope.tape, 0, 0, 0, dsVirtualEnd);

      bool mismatch = false;
      compileTypecast(scope, target, scope.moduleScope->getBoolType(), mismatch, HINT_TYPEENFORCING);

      _writer.jumpIfNotEqual(*scope.tape, (verb == IF_MESSAGE_ID) ? scope.moduleScope->trueReference : scope.moduleScope->falseReference);
   }

   _writer.declareBlock(*scope.tape);

   CodeScope subScope(&scope);

   DNode thenCode = thenNode.firstChild();
   if (thenCode.firstChild().nextNode() != nsNone) {
      compileCode(thenCode, subScope, subCodeMode );
   }
   // if it is inline action
   else compileRetExpression(thenCode.firstChild(), scope, 0);

   return ObjectInfo(okAccumulator);
}

void Compiler :: compileThrow(DNode node, CodeScope& scope, int mode)
{
   compileExpression(node.firstChild(), scope, mode);
   _writer.throwCurrent(*scope.tape);
}

void Compiler :: compileBreak(DNode node, CodeScope& scope, int mode)
{
   ObjectInfo retVal(okConstantSymbol, scope.moduleScope->nilReference);
   DNode breakExpr = node.firstChild();
   // if break expression is provided
   if (breakExpr != nsNone) {
      retVal = compileExpression(breakExpr, scope, mode);
   }
   // otherwise use nil
   _writer.loadObject(*scope.tape, retVal);

   _writer.breakLoop(*scope.tape, -1);

   MethodScope* methodScope = (MethodScope*)scope.getScope(Scope::slMethod);
   methodScope->withBreakHandler = true;
}

void Compiler :: compileLoop(DNode node, CodeScope& scope, int mode)
{
   _writer.declareLoop(*scope.tape/*, true*/);

   DNode expr = node.firstChild().firstChild();

   // if it is while-do loop
   if (expr.nextNode() == nsL7Operation) {
      DNode loopNode = expr.nextNode();

      ObjectInfo cond = compileObject(expr, scope, mode);

      // get the current value
      _writer.loadObject(*scope.tape, cond);

      compileBranching(loopNode, scope, cond, IF_MESSAGE_ID, 0);

      _writer.endLoop(*scope.tape);
   }
   // if it is do-until loop
   else if (expr.firstChild() == nsSubCode) {
      CodeScope subScope(&scope);

      DNode code = expr.firstChild();

      compileCode(code, subScope);

      _writer.declareBreakpoint(*scope.tape, 0, 0, 0, dsVirtualEnd);

      bool mismatch = false;
      compileTypecast(scope, ObjectInfo(okAccumulator), scope.moduleScope->getBoolType(), mismatch, HINT_TYPEENFORCING);

      _writer.endLoop(*scope.tape, scope.moduleScope->trueReference);
   }
   // if it is repeat loop
   else {
      ObjectInfo retVal = compileExpression(expr, scope, mode);

      _writer.declareBreakpoint(*scope.tape, 0, 0, 0, dsVirtualEnd);

      bool mismatch = false;
      compileTypecast(scope, retVal, scope.moduleScope->getBoolType(), mismatch, HINT_TYPEENFORCING);

      _writer.endLoop(*scope.tape, scope.moduleScope->trueReference);
   }
}

void Compiler :: compileCode(DNode node, CodeScope& scope, int mode)
{
   bool needVirtualEnd = true;
   DNode statement = node.firstChild();

   // skip subject argument
   while (statement == nsSubjectArg || statement == nsMethodParameter)
      statement= statement.nextNode();

   while (statement != nsNone) {
      DNode hints = skipHints(statement);

      _writer.declareStatement(*scope.tape);

      switch(statement) {
         case nsExpression:
            compileExpression(statement, scope, HINT_ROOTEXPR);
            break;
         case nsThrow:
            compileThrow(statement, scope, 0);
            break;
         case nsBreak:
            compileBreak(statement, scope, 0);
            break;
         case nsLoop:
            compileLoop(statement, scope, HINT_LOOP);
            break;
         case nsRetStatement:
         {
            needVirtualEnd = false;
            compileRetExpression(statement.firstChild(), scope, 0);
            scope.freeSpace();

            _writer.gotoEnd(*scope.tape, test(mode, HINT_SUBBRANCH) ? baPreviousLabel : baCurrentLabel);
            break;
         }
         case nsVariable:
            compileVariable(statement, scope, hints);
            break;
         case nsCodeEnd:
            needVirtualEnd = false;
            recordStep(scope, statement.Terminal(), dsEOP);
            break;
      }
      scope.freeSpace();

      statement = statement.nextNode();
   }

   if (needVirtualEnd)
      _writer.declareBreakpoint(*scope.tape, 0, 0, 0, dsVirtualEnd);
}

void Compiler :: compileExternalArguments(DNode arg, CodeScope& scope, ExternalScope& externalScope)
{
   ModuleScope* moduleScope = scope.moduleScope;

   ref_t actionType = moduleScope->getActionType();

   while (arg == nsSubjectArg) {
      TerminalInfo terminal = arg.Terminal();

      ExternalScope::ParamInfo param;
      param.subject = moduleScope->mapType(terminal, param.output);

      int size = moduleScope->sizeHints.get(param.subject);
      // HOTFIX: allow literal argument to be passed as external call argument
      if (size == 0 && param.subject == moduleScope->getLiteralType()) {
         size = -2;
      }

      arg = arg.nextNode();
      if (arg == nsTypedMessageParameter || arg == nsMessageParameter) {
         param.info = compileObject(arg.firstChild(), scope, 0);

         // only local variables can be used as output parameters
         if (param.output) {
            if(param.info.kind != okLocal || param.subject != param.info.extraparam)
               scope.raiseError(errInvalidOperation, terminal);
         }            
         else if (arg == nsTypedMessageParameter) {         
            _writer.loadObject(*scope.tape, param.info);
            bool dummy;
            param.info = saveObject(scope, compileTypecast(scope, param.info, param.subject, dummy, HINT_TYPEENFORCING), 0);
            param.info.kind = okBlockLocal;
            param.info.param = ++externalScope.frameSize;
         }
         else if (param.info.kind == okIntConstant && size == 4 || param.subject == actionType && param.info.kind == okSymbolReference) {
            // if direct pass is possible
            // do nothing at this stage
         }
         else if(param.subject == param.info.extraparam || size == -1 || param.subject == actionType) {
            saveObject(scope, param.info, 0);
            param.info.kind = okBlockLocal;
            param.info.param = ++externalScope.frameSize;
         }
         else {
            _writer.loadObject(*scope.tape, param.info);
            bool mismatch = false;
            param.info = saveObject(scope, 
               compileTypecast(scope, ObjectInfo(okAccumulator, 0, param.info.extraparam), param.subject, mismatch, HINT_TYPEENFORCING), 0);

            if (mismatch)
               scope.raiseWarning(wrnTypeMismatch, terminal);

            param.info.kind = okBlockLocal;
            param.info.param = ++externalScope.frameSize;
         }

         arg = arg.nextNode();
      }
      else scope.raiseError(errInvalidOperation, terminal);

      // raise an error if the subject type is not supported
      if ((param.output && size != 0) || size == 4 || size < 0 || param.subject == actionType) {
      }
      else scope.raiseError(errInvalidOperation, terminal);

      externalScope.operands.push(param);
   }
}

void Compiler :: saveExternalParameters(CodeScope& scope, ExternalScope& externalScope)
{
   ModuleScope* moduleScope = scope.moduleScope;

   ref_t actionType = moduleScope->getActionType();

   // save function parameters
   Stack<ExternalScope::ParamInfo>::Iterator out_it = externalScope.operands.start();
   while (!out_it.Eof()) {
      // if it is output parameter
      if ((*out_it).output) {
         _writer.pushObject(*scope.tape, (*out_it).info);
      }
      else if ((*out_it).subject == actionType) {
         _writer.loadSymbolReference(*scope.tape, (*out_it).info.param);
         _writer.pushObject(*scope.tape, ObjectInfo(okAccumulator));
      }
      else {
         int size = moduleScope->sizeHints.get((*out_it).subject);
         if (size == 4) {
            if ((*out_it).info.kind == okIntConstant) {
               int value = StringHelper::strToInt(moduleScope->module->resolveConstant((*out_it).info.param));

               externalScope.frameSize++;
               _writer.declarePrimitiveVariable(*scope.tape, value);
            }
            else {
               _writer.loadObject(*scope.tape, (*out_it).info);
               _writer.pushObject(*scope.tape, ObjectInfo(okAccField, 0));
            }
         }
         else _writer.pushObject(*scope.tape, (*out_it).info);
      }

      out_it++;
   }
}

ObjectInfo Compiler :: compileExternalCall(DNode node, CodeScope& scope, const wchar16_t* dllName, int mode)
{
   ObjectInfo retVal(okIndexAccumulator);

   ModuleScope* moduleScope = scope.moduleScope;

   ReferenceNs name(DLL_NAMESPACE);
   name.combine(dllName + strlen(EXTERNAL_MODULE) + 1);
   name.append(".");
   name.append(node.Terminal());

   ref_t reference = moduleScope->module->mapReference(name);

   // compile argument list
   ExternalScope externalScope;

   _writer.declareExternalBlock(*scope.tape);

   compileExternalArguments(node.firstChild(), scope, externalScope);

   // save function parameters
   saveExternalParameters(scope, externalScope);

   // call the function
   _writer.callExternal(*scope.tape, reference, externalScope.frameSize);

   //// indicate that the result is 0 or -1
   //if (test(mode, HINT_LOOP))
   //   retVal.extraparam = scope.moduleScope->intSubject;

   // error handling should follow the function call immediately
   if (test(mode, HINT_TRY))
      compilePrimitiveCatch(node.nextNode(), scope);

  _writer.endExternalBlock(*scope.tape);

   return retVal;
}

void Compiler :: reserveSpace(CodeScope& scope, int size)
{
   MethodScope* methodScope = (MethodScope*)scope.getScope(Scope::slMethod);

   // if it is not enough place to allocate
   if (methodScope->reserved < scope.reserved) {
      ByteCodeIterator allocStatement = scope.tape->find(bcOpen);
      // reserve place for stack allocated object
      (*allocStatement).argument += size;

      // if stack was not allocated before
      // update method enter code
      if (methodScope->reserved == 0) {
         // to include new frame header
         (*allocStatement).argument += 2;

         _writer.insertStackAlloc(allocStatement, *scope.tape, size);
      }
      // otherwise update the size
      else _writer.updateStackAlloc(allocStatement, *scope.tape, size);

      methodScope->reserved += size;
   }
}

bool Compiler :: allocateStructure(CodeScope& scope, int dynamicSize, ObjectInfo& exprOperand)
{
   bool bytearray = false;
   // !! temporal
   int size = scope.moduleScope->sizeHints.get(exprOperand.extraparam);
   if (size < 0) {
      bytearray = true;

      // plus space for size
      size = ((dynamicSize + 3) >> 2) + 2;
   }
   else if (size == 0) {
      return false;
   }
   else size = (size + 3) >> 2;

   if (size > 0) {
      MethodScope* methodScope = (MethodScope*)scope.getScope(Scope::slMethod);

      exprOperand.kind = okLocalAddress;
      exprOperand.param = scope.newSpace(size);

      // allocate
      reserveSpace(scope, size);

      // reserve place for byte array header if required
      if (bytearray) {
         _writer.loadObject(*scope.tape, exprOperand);
         _writer.saveIntConstant(*scope.tape, -dynamicSize);

         exprOperand.param -= 2;
      }

      // indicate the frame usage
      // to prevent commenting frame operation out
      methodScope->masks = MTH_FRAME_USED;

      return true;
   }
   else return false;
}

inline void copySubject(_Module* module, ReferenceNs& signature, ref_t type)
{
   signature.append(module->resolveSubject(type));
}

inline bool IsVarOperation(int operator_id)
{
   switch (operator_id) {
      case WRITE_MESSAGE_ID:
      case APPEND_MESSAGE_ID:
      case REDUCE_MESSAGE_ID:
      case SEPARATE_MESSAGE_ID:
      case INCREASE_MESSAGE_ID:
         return true;
      default:
         return false;
   }
}

ObjectInfo Compiler :: compilePrimitiveCatch(DNode node, CodeScope& scope)
{
   _writer.declarePrimitiveCatch(*scope.tape);

   size_t size = 0;
   ref_t message = mapMessage(node, scope, ObjectInfo(okIndexAccumulator), size);
   if (message == encodeMessage(0, RAISE_MESSAGE_ID, 1)) {
      compileThrow(node.firstChild().firstChild(), scope, 0);
   }
   else scope.raiseError(errInvalidOperation, node.Terminal());

   _writer.endPrimitiveCatch(*scope.tape);

   return ObjectInfo(okIndexAccumulator);
}

ref_t Compiler :: declareInlineArgumentList(DNode arg, MethodScope& scope)
{
   IdentifierString signature;

   ref_t sign_id = 0;

   // if method has generic (unnamed) argument list
   while (arg == nsMethodParameter || arg == nsObject) {
      TerminalInfo paramName = arg.Terminal();
      int index = 1 + scope.parameters.Count();
      scope.parameters.add(paramName, Parameter(index));

      arg = arg.nextNode();
   }
   bool first = true;
   while (arg == nsSubjectArg) {
      TerminalInfo subject = arg.Terminal();

      if (!first) {
         signature.append('&');
      }
      else first = false;

      bool out = false;
      ref_t subj_ref = scope.moduleScope->mapType(subject, out);
      
      signature.append(subject);

      // declare method parameter
      arg = arg.nextNode();
         
      if (arg == nsMethodParameter) {
         // !! check duplicates

         int index = 1 + scope.parameters.Count();
         scope.parameters.add(arg.Terminal(), Parameter(index, subj_ref, out));

         arg = arg.nextNode();
      }
   }

   if (!emptystr(signature))
      sign_id = scope.moduleScope->module->mapSubject(signature, false);

   return encodeMessage(sign_id, EVAL_MESSAGE_ID, scope.parameters.Count());
}

void Compiler :: declareArgumentList(DNode node, MethodScope& scope)
{
   IdentifierString signature;

   TerminalInfo verb = node.Terminal();
   ref_t verb_id = _verbs.get(verb.value);
   ref_t sign_id = 0;

   // if it is a generic verb, make sure no parameters are provided
   if (verb_id == DISPATCH_MESSAGE_ID) {
      scope.raiseError(errInvalidOperation, verb);
   }

   DNode arg = node.firstChild();

   bool first = true;
   int paramCount = 0;

   if (verb_id == 0) {
      // add a namespace for the private message
      if (verb == tsPrivate) {
         signature.append(scope.moduleScope->project->StrSetting(opNamespace));
      }

      //scope.withCustomVerb = true;

      signature.append(verb);

      // if followed by argument list - it is a EVAL verb
      if (arg == nsSubjectArg || arg == nsMethodParameter || arg == nsMethodOpenParameter) {
         verb_id = EVAL_MESSAGE_ID;
         first = false;
      }
      // otherwise it is GET message
      else verb_id = GET_MESSAGE_ID;
   }

   // if method has generic (unnamed) argument list
   while (arg == nsMethodParameter) {
      int index = 1 + scope.parameters.Count();

      if (scope.parameters.exist(arg.Terminal()))
         scope.raiseError(errDuplicatedLocal, arg.Terminal());

      scope.parameters.add(arg.Terminal(), Parameter(index));
      paramCount++;

      arg = arg.nextNode();
   }

   // if method has an open argument list
   if (arg == nsMethodOpenParameter) {
      scope.parameters.add(arg.Terminal(), Parameter(1 + scope.parameters.Count(), scope.moduleScope->getParamsType()));

      // if the method contains the generic parameters
      if (paramCount > 0) {
         // add the special postfix
         if (!first)
            signature.append('&');

         signature.appendInt(paramCount);

         // the generic arguments should be free by the method exit
         scope.rootToFree += paramCount;
      }
      // to indicate open argument list
      paramCount = OPEN_ARG_COUNT;
   }
   else {
      // if method has named argument list
      while (arg == nsSubjectArg) {
         TerminalInfo subject = arg.Terminal();

         if (!first) {
            signature.append('&');
         }
         else first = false;

         bool out = false;
         ref_t subj_ref = scope.moduleScope->mapType(subject, out);

         signature.append(subject);

         arg = arg.nextNode();

         if (arg == nsMethodParameter) {
            if (scope.parameters.exist(arg.Terminal()))
               scope.raiseError(errDuplicatedLocal, arg.Terminal());

            int index = 1 + scope.parameters.Count();

            paramCount++;
            scope.parameters.add(arg.Terminal(), Parameter(index, subj_ref, out));

            arg = arg.nextNode();
         }
      }

      if (paramCount >= OPEN_ARG_COUNT)
         scope.raiseError(errTooManyParameters, verb);
   }

   // if signature is presented
   if (!emptystr(signature)) {
      sign_id = scope.moduleScope->module->mapSubject(signature, false);
   }

   // declare method parameter debug info
   //_writer.declareLocalInfo(*codeScope.tape, SELF_VAR, -5);

   scope.message = encodeMessage(sign_id, verb_id, paramCount);
}

void Compiler :: compileDispatcher(DNode node, MethodScope& scope)
{
   bool genericDispatch = getVerb(scope.message) == GENERIC_MESSAGE_ID;

   // check if the method is inhreited and update vmt size accordingly
   scope.include();

   CodeScope codeScope(&scope);

   _writer.declareIdleMethod(*codeScope.tape, scope.message);

   if (node == nsImport) {
      ReferenceNs routine(PACKAGE_MODULE, INLINE_MODULE);

      if (genericDispatch) {
         routine.combine("generic_");
         routine.append(node.Terminal());
      }
      else routine.combine(node.Terminal());

      importCode(node, *scope.moduleScope, codeScope.tape, routine);
   }
   else {
      _writer.doGenericHandler(*codeScope.tape, genericDispatch);

      DNode nextNode = node.nextNode();

      // !! currently only simple construction is supported
      if (node == nsObject && node.firstChild() == nsNone && nextNode == nsNone) {
         ObjectInfo extension = compileTerminal(node, codeScope, 0);
         ClassScope* classScope = (ClassScope*)scope.getScope(Scope::slClass);

         _writer.resend(*codeScope.tape, extension, genericDispatch ? 1 : 0);
      }
      else scope.raiseError(errInvalidOperation, node.Terminal());
   }

   _writer.endIdleMethod(*codeScope.tape);
}

void Compiler :: compileAction(DNode node, MethodScope& scope)
{
   // check if the method is inhreited and update vmt size accordingly
   scope.include();

   CodeScope codeScope(&scope);

   // new stack frame
   // stack already contains previous $self value
   _writer.declareMethod(*codeScope.tape, scope.message);
   codeScope.level++;

//   declareParameterDebugInfo(scope, codeScope.tape, false);

   compileCode(node, codeScope);

   if (scope.withBreakHandler) {
      _writer.exitMethod(*codeScope.tape, scope.parameters.Count() + 1, scope.reserved);

      compileBreakHandler(codeScope, 0);
      _writer.endIdleMethod(*codeScope.tape);
   }
   else _writer.endMethod(*codeScope.tape, scope.parameters.Count() + 1, scope.reserved);
}

void Compiler :: compileExpressionAction(DNode node, MethodScope& scope)
{
   // check if the method is inhreited and update vmt size accordingly
   scope.include();

   // stack already contains previous $self value
   CodeScope codeScope(&scope);

   // new stack frame
   // stack already contains previous $self value
   _writer.declareMethod(*codeScope.tape, scope.message);
   codeScope.level++;

   declareParameterDebugInfo(scope, codeScope.tape, false);

   compileRetExpression(node, codeScope, 0);

   if (scope.withBreakHandler) {
      _writer.exitMethod(*codeScope.tape, scope.parameters.Count() + 1, scope.reserved);
      compileBreakHandler(codeScope, 0);
      _writer.endIdleMethod(*codeScope.tape);
   }
   else _writer.endMethod(*codeScope.tape, scope.parameters.Count() + 1, scope.reserved);
}

void Compiler :: compileDispatchExpression(DNode node, CodeScope& scope)
{
   MethodScope* methodScope = (MethodScope*)scope.getScope(Scope::slMethod);

   // try to implement light-weight resend operation
   if (node.firstChild() == nsNone && node.nextNode() == nsNone) {
      ObjectInfo target = compileTerminal(node, scope, 0);
      if (target.kind == okConstant || target.kind == okField) {
         _writer.declareMethod(*scope.tape, methodScope->message, false);

         if (target.kind == okField) {
            _writer.loadObject(*scope.tape, ObjectInfo(okAccField, target.param));
         }
         else _writer.loadObject(*scope.tape, target);

         _writer.resend(*scope.tape);

         _writer.endMethod(*scope.tape, getParamCount(methodScope->message) + 1, methodScope->reserved, false);

         return;
      }
   }

   // new stack frame
   // stack already contains previous $self value
   _writer.declareMethod(*scope.tape, methodScope->message, true);
   scope.level++;

   _writer.pushObject(*scope.tape, ObjectInfo(okParam, -1));
   _writer.pushObject(*scope.tape, ObjectInfo(okExtraRegister));

   ObjectInfo target = compileObject(node, scope, 0);
   if (checkIfBoxingRequired(scope, target))
      boxObject(scope, target, 0);

   _writer.loadObject(*scope.tape, target);

   _writer.popObject(*scope.tape, ObjectInfo(okExtraRegister));

   _writer.callRoleMessage(*scope.tape, getParamCount(methodScope->message));

   _writer.endMethod(*scope.tape, getParamCount(methodScope->message) + 1, methodScope->reserved, true);
}

void Compiler :: compileResendExpression(DNode node, CodeScope& scope)
{
   MethodScope* methodScope = (MethodScope*)scope.getScope(Scope::slMethod);

   // new stack frame
   // stack already contains current $self reference
   _writer.declareMethod(*scope.tape, methodScope->message, true);
   scope.level++;

   compileMessage(node, scope, ObjectInfo(okThisParam, 1, methodScope->getClassType()), 0);
   scope.freeSpace();

   _writer.declareBreakpoint(*scope.tape, 0, 0, 0, dsVirtualEnd);

   _writer.endMethod(*scope.tape, getParamCount(methodScope->message) + 1, methodScope->reserved, true);
}

void Compiler :: compileBreakHandler(CodeScope& scope, int mode)
{
   MethodScope* methodScope = (MethodScope*)scope.getScope(Scope::slMethod);

   ref_t vmtRef = scope.moduleScope->mapConstantReference(BREAK_EXCEPTION_CLASS);

   scope.tape->setPredefinedLabel(-1);
   _writer.pushObject(*scope.tape, ObjectInfo(okAccumulator));

   _writer.newObject(*scope.tape, 1, vmtRef);
   _writer.loadBase(*scope.tape, ObjectInfo(okAccumulator));
   _writer.saveBase(*scope.tape, ObjectInfo(okCurrent), 0);
   _writer.loadObject(*scope.tape, ObjectInfo(okBase));

   _writer.throwCurrent(*scope.tape);
}

void Compiler :: compileSpecialMethod(MethodScope& scope)
{
   CodeScope codeScope(&scope);

   scope.include();
   _writer.declareIdleMethod(*codeScope.tape, scope.message);

   if (scope.message == encodeVerb(DISPATCH_MESSAGE_ID)) {
      ClassScope* classScope = (ClassScope*)scope.getScope(Scope::slClass);

      if (test(classScope->info.header.flags, elWithGenerics)) {
         _writer.doGenericHandler(*codeScope.tape, encodeMessage(scope.moduleScope->mapSubject(GENERIC_POSTFIX), 0, 0));
      }
   }

   _writer.endIdleMethod(*codeScope.tape);
}

void Compiler :: compileImportMethod(DNode node, ClassScope& scope, ref_t message, const char* function)
{
   MethodScope methodScope(&scope);
   methodScope.message = message;

   methodScope.include();

   CodeScope codeScope(&methodScope);

   compileImportMethod(node, codeScope, message, ConstantIdentifier(function), 0);
}

void Compiler :: compileImportMethod(DNode node, CodeScope& codeScope, ref_t message, const wchar16_t* function, int mode)
{
   ReferenceNs reference(PACKAGE_MODULE, INLINE_MODULE);
   reference.combine(function);

   _writer.declareIdleMethod(*codeScope.tape, message);
   importCode(node, *codeScope.moduleScope, codeScope.tape, reference);
   _writer.endIdleMethod(*codeScope.tape);
}

void Compiler :: compileMethod(DNode node, MethodScope& scope, int mode)
{
   // check if the method is inhreited and update vmt size accordingly
   scope.include();

   int paramCount = getParamCount(scope.message);

   CodeScope codeScope(&scope);

   // save extensions if any
   if (test(codeScope.getClassFlags(false), elExtension)) {
      codeScope.moduleScope->saveExtension(scope.message, codeScope.getExtensionType(), codeScope.getClassRefId());
   }

   DNode resendBody = node.select(nsResendExpression);
   DNode dispatchBody = node.select(nsDispatchExpression);
   DNode importBody = node.select(nsImport);

   // check if it is resend
   if (importBody == nsImport) {
      compileImportMethod(importBody, codeScope, scope.message, importBody.Terminal(), mode);
   }
   // check if it is a resend
   else if (resendBody != nsNone) {         
      compileResendExpression(resendBody.firstChild(), codeScope);
   }
   // check if it is a dispatch
   else if (dispatchBody != nsNone) {
      compileDispatchExpression(dispatchBody.firstChild(), codeScope);
   }
   else {
      // new stack frame
      // stack already contains current $self reference
      if (test(mode, HINT_GENERIC_METH)) {
         _writer.declareGenericMethod(*codeScope.tape, scope.message);
      }
      else _writer.declareMethod(*codeScope.tape, scope.message);
      codeScope.level++;

      declareParameterDebugInfo(scope, codeScope.tape, true);

      DNode body = node.select(nsSubCode);
      // if method body is a return expression
      if (body==nsNone) {
         compileCode(node, codeScope);
      }
      // if method body is a set of statements
      else {
         compileCode(body, codeScope);

         _writer.loadObject(*codeScope.tape, ObjectInfo(okThisParam, 1));
      }

      int stackToFree = paramCount + scope.rootToFree;

   //   if (scope.testMode(MethodScope::modLock)) {
   //      _writer.endSyncMethod(*codeScope.tape, -1);
   //   }
      if (scope.withBreakHandler) {
         _writer.exitMethod(*codeScope.tape, stackToFree, scope.reserved);
         compileBreakHandler(codeScope, 0);
         _writer.endIdleMethod(*codeScope.tape);
      }
      else _writer.endMethod(*codeScope.tape, stackToFree, scope.reserved);
   }

//   // critical section entry if sync hint declared
//   if (scope.testMode(MethodScope::modLock)) {
//      ownerScope->info.header.flags |= elWithLocker;
//
//      _writer.tryLock(*codeScope.tape, 1);
//   }
//   // safe point (no need in extra one because lock already has one safe point)
//   else if (scope.testMode(MethodScope::modSafePoint))
//      _writer.declareSafePoint(*codeScope.tape);
}

void Compiler :: compileConstructor(DNode node, MethodScope& scope, ClassScope& classClassScope, DNode hints)
{
   ClassScope* classScope = (ClassScope*)scope.getScope(Scope::slClass);

   // check if the method is inhreited and update vmt size accordingly
   // NOTE: the method is class class member though it is compiled within class scope
   ClassInfo::MethodMap::Iterator it = classClassScope.info.methods.getIt(scope.message);
   if (it.Eof()) {
      classClassScope.info.methods.add(scope.message, true);
   }
   else (*it) = true;

   CodeScope codeScope(&scope);

//   // compile constructor hints
//   int mode = scope.compileHints(hints);

   // HOTFIX: constructor is declared in class class but should be executed if the class instance
   codeScope.tape = &classClassScope.tape;

   if (getVerb(scope.message) == NEWOBJECT_MESSAGE_ID) {
      compileDefaultConstructor(node.firstChild().firstChild(), scope, classClassScope, hints);
   }
   else {
      DNode body = node.select(nsSubCode);
      DNode resendBody = node.select(nsResendExpression);

      if (resendBody != nsNone) {
         compileResendExpression(resendBody.firstChild(), codeScope);
      }
      else {
         _writer.declareMethod(*codeScope.tape, scope.message, false);

         // HOTFIX: -1 indicates the stack is not consumed by the constructor
         _writer.callMethod(*codeScope.tape, 2, -1);

         DNode importBody = node.select(nsImport);

         // check if it is resend
         if (importBody == nsImport) {
            ReferenceNs reference(PACKAGE_MODULE, INLINE_MODULE);
            reference.combine(importBody.Terminal());

            importCode(importBody, *scope.moduleScope, codeScope.tape, reference);
            _writer.endIdleMethod(*codeScope.tape);
         }
         else {
            // new stack frame
            // stack already contains $self value
            _writer.newFrame(*codeScope.tape);
            codeScope.level++;

            declareParameterDebugInfo(scope, codeScope.tape, true);

            compileCode(body, codeScope);

            _writer.loadObject(*codeScope.tape, ObjectInfo(okThisParam, 1));

            _writer.endMethod(*codeScope.tape, getParamCount(scope.message) + 1, scope.reserved);
         }
      }
   }
}

void Compiler :: compileDefaultConstructor(DNode node, MethodScope& scope, ClassScope& classClassScope, DNode hints)
{
   ClassScope* classScope = (ClassScope*)scope.getScope(Scope::slClass);

   // check if the method is inhreited and update vmt size accordingly
   // NOTE: the method is class class member though it is compiled within class scope
   ClassInfo::MethodMap::Iterator it = classClassScope.info.methods.getIt(scope.message);
   if (it.Eof()) {
      classClassScope.info.methods.add(scope.message, true);
   }
   else (*it) = true;

   CodeScope codeScope(&scope);

   // compile constructor hints
   //int mode = scope.compileHints(hints);

   // HOTFIX: constructor is declared in class class but should be executed if the class instance
   codeScope.tape = &classClassScope.tape;

   _writer.declareIdleMethod(*codeScope.tape, scope.message);

   // if it has custom implementation
   if (node == nsImport) {
      ReferenceNs reference(PACKAGE_MODULE, INLINE_MODULE);
      reference.combine(node.Terminal());

      importCode(node, *scope.moduleScope, codeScope.tape, reference);
   }
   // if it has default implementation
   else {
      if (test(classScope->info.header.flags, elStructureRole)) {
         if (!test(classScope->info.header.flags, elDynamicRole)) {
            _writer.newStructure(*codeScope.tape, classScope->info.size, classScope->reference);
         }
      }
      else if (!test(classScope->info.header.flags, elDynamicRole)) {
         _writer.newObject(*codeScope.tape, classScope->info.fields.Count(), classScope->reference);
         _writer.loadBase(*codeScope.tape, ObjectInfo(okAccumulator));
         _writer.loadObject(*codeScope.tape, ObjectInfo(okConstantSymbol, scope.moduleScope->nilReference));
         _writer.initBase(*codeScope.tape, classScope->info.fields.Count());
         _writer.loadObject(*codeScope.tape, ObjectInfo(okBase));
      }

      _writer.exitMethod(*codeScope.tape, 0, 0, false);
   }

   _writer.endIdleMethod(*codeScope.tape);
}

void Compiler :: compileVMT(DNode member, ClassScope& scope)
{
   int inheritedFlags = scope.info.header.flags;

   while (member != nsNone) {
      DNode hints = skipHints(member);

      switch(member) {
         case nsMethod:
         {
            MethodScope methodScope(&scope);

            // if it is a dispatch handler
            if (member.firstChild() == nsDispatchHandler) {
               if (test(scope.info.header.flags, elRole))
                  scope.raiseError(errInvalidRoleDeclr, member.Terminal());

               methodScope.message = encodeVerb(DISPATCH_MESSAGE_ID);

               // check if there is no duplicate method
               if (scope.info.methods.exist(methodScope.message, true))
                  scope.raiseError(errDuplicatedMethod, member.Terminal());

               compileDispatcher(member.firstChild().firstChild(), methodScope);

               // NOTE: due to the current implementation - there are two dispatch handler (second one - typed dispatch)
               methodScope.message = encodeVerb(GENERIC_MESSAGE_ID);
               compileDispatcher(member.firstChild().firstChild(), methodScope);
            }
            // if it is a normal method
            else {
               declareArgumentList(member, methodScope);

               // check if there is no duplicate method
               if (scope.info.methods.exist(methodScope.message, true))
                  scope.raiseError(errDuplicatedMethod, member.Terminal());

               compileMethod(member, methodScope, methodScope.compileHints(hints));
            }
            break;
         }
         case nsGeneric:
         case nsDefaultGeneric:
         {
            MethodScope methodScope(&scope);
            declareArgumentList(member, methodScope);

            // override subject with generic postfix
            methodScope.message = overwriteSubject(methodScope.message, scope.moduleScope->mapSubject(GENERIC_POSTFIX));

            // mark as having generic methods
            scope.info.header.flags |= elWithGenerics;

            compileMethod(member, methodScope, HINT_GENERIC_METH);
            break;
         }
      }
      member = member.nextNode();
   }

   // if the VMT conatains newly defined generic handlers, overrides default one
   if (test(scope.info.header.flags, elWithGenerics) && !test(inheritedFlags, elWithGenerics)) {
      MethodScope methodScope(&scope);
      methodScope.message = encodeVerb(DISPATCH_MESSAGE_ID);

      compileSpecialMethod(methodScope);
   }
}

void Compiler :: compileFieldDeclarations(DNode& member, ClassScope& scope)
{
   while (member != nsNone) {
      DNode hints = skipHints(member);

      if (member==nsField) {
         // a role cannot have fields
         if (test(scope.info.header.flags, elRole))
            scope.raiseError(errIllegalField, member.Terminal());

         // a class with a dynamic length structure must have no fields
         if (test(scope.info.header.flags, elDynamicRole))
            scope.raiseError(errIllegalField, member.Terminal());

         if (scope.info.fields.exist(member.Terminal()))
            scope.raiseError(errDuplicatedField, member.Terminal());

         int sizeValue = 0;
         ref_t typeRef = 0;
         scope.compileFieldHints(hints, sizeValue, typeRef);

         int offset = scope.info.size;
         // if it is a data type and structural field is possible
         if (sizeValue != 0 && (test(scope.info.header.flags, elStructureRole) || scope.info.fields.Count() == 0)) {
            // if it is a dynamic array
            if (sizeValue == (size_t)-4 && typeRef == (size_t)-1) {
               scope.info.header.flags |= elDynamicRole;

               typeRef = sizeValue = 0; // !! to indicate dynamic object
            }
            // if it is a char array / literal
            else if (sizeValue == (size_t)-2) {
               scope.info.header.flags |= elDynamicRole;
               scope.info.header.flags |= elStructureRole;
               scope.info.size = sizeValue;
            }
            // if it is a dynamic byte array
            else if (sizeValue == (size_t)-1) {
               scope.info.header.flags |= elDynamicRole;
               scope.info.header.flags |= elStructureRole;
               scope.info.size = sizeValue;
            }
            // if it is a data field
            else {
               scope.info.header.flags |= elStructureRole;
               scope.info.size += sizeValue;
            }

            // do not add primitive fields
            if (typeRef != 0) {
               scope.info.fields.add(member.Terminal(), offset);

               scope.info.fieldTypes.add(offset, typeRef);
            }
         }
         // if it is a normal field
         else {
            if (test(scope.info.header.flags, elStructureRole))
               scope.raiseError(errIllegalField, member.Terminal());

            int offset = scope.info.fields.Count();
            scope.info.fields.add(member.Terminal(), offset);

            if (typeRef != 0)
               scope.info.fieldTypes.add(offset, typeRef);
         }
      }
      else {
         // due to current syntax we need to reset hints back, otherwise they will be skipped
         if (hints != nsNone)
            member = hints;

         break;
      }
      member = member.nextNode();
   }
}

void Compiler :: compileSymbolCode(ClassScope& scope)
{
   // creates implicit symbol
   SymbolScope symbolScope(scope.moduleScope, scope.reference);

   _writer.declareSymbol(symbolScope.tape, symbolScope.reference);
   _writer.loadObject(symbolScope.tape, ObjectInfo(okConstantClass, scope.reference));
   _writer.endSymbol(symbolScope.tape);

   // create byte code sections
   _writer.compile(symbolScope.tape, scope.moduleScope->module, scope.moduleScope->debugModule, scope.moduleScope->sourcePathRef);
}

void Compiler :: compileClassClassDeclaration(DNode node, ClassScope& classClassScope, ClassScope& classScope)
{
   _writer.declareClass(classClassScope.tape, classClassScope.reference);

   // inherit class class parent
   if (classScope.info.header.parentRef != 0) {
      ref_t superClass = classClassScope.moduleScope->mapConstantReference(SUPER_CLASS);
      // NOTE: if it is a super class direct child
      //       super class is used as a base for its class class
      //       otherwise class class should be inherited
      if (classScope.info.header.parentRef != superClass) {
         IdentifierString classClassParentName(classClassScope.moduleScope->module->resolveReference(classScope.info.header.parentRef));
         classClassParentName.append(CLASSCLASS_POSTFIX);

         classClassScope.info.header.parentRef = classClassScope.moduleScope->module->mapReference(classClassParentName);
      }
      else classClassScope.info.header.parentRef = superClass;
   }

   InheritResult res = inheritClass(classClassScope, classClassScope.info.header.parentRef);
   //if (res == irObsolete) {
   //   scope.raiseWarning(wrnObsoleteClass, node.Terminal());
   //}
   if (res == irInvalid) {
      classClassScope.raiseError(errInvalidParent, node.Terminal());
   }
   else if (res == irSealed) {
      classClassScope.raiseError(errSealedParent, node.Terminal());
   }
   else if (res == irUnsuccessfull)
      classClassScope.raiseError(node != nsNone ? errUnknownClass : errUnknownBaseClass, node.Terminal());

   // class class is always stateless
   classClassScope.info.header.flags |= elStateless;

   DNode member = node.firstChild();
   // compile constructors
   while (member != nsNone) {
      DNode hints = skipHints(member);

      if (member == nsConstructor) {
         MethodScope methodScope(&classScope);

         if (member.firstChild() == nsConstructorExpression) {
            if (test(classScope.info.header.flags, elDynamicRole)) {
               methodScope.message = encodeMessage(0, NEWOBJECT_MESSAGE_ID, 1);
            }
            else methodScope.message = encodeVerb(NEWOBJECT_MESSAGE_ID);
         }
         else declareArgumentList(member, methodScope);

         // check if there is no duplicate method
         if (classClassScope.info.methods.exist(methodScope.message, true))
            classClassScope.raiseError(errDuplicatedMethod, member.Terminal());

         if (test(classScope.info.header.flags, elStateless))
            classClassScope.raiseError(errInvalidOperation, member.Terminal());

         compileConstructor(member, methodScope, classClassScope, hints);
      }
      member = member.nextNode();
   }

   if (!test(classScope.info.header.flags, elStateless)) {
      if (test(classScope.info.header.flags, elDynamicRole)) {
         // if no custom default constructor defined - raise an error
         if (!classClassScope.info.methods.exist(encodeMessage(0, NEWOBJECT_MESSAGE_ID, 1), true))
            classClassScope.raiseError(errNotDefaultConstructor, node.FirstTerminal());
      }
      // if no custom default constructor defined - autogenerate one
      else if (!classClassScope.info.methods.exist(encodeVerb(NEWOBJECT_MESSAGE_ID), true)) {
         MethodScope methodScope(&classScope);
         methodScope.message = encodeVerb(NEWOBJECT_MESSAGE_ID);

         compileDefaultConstructor(DNode(), methodScope, classClassScope, DNode());
      }
   }

   _writer.endClass(classClassScope.tape);

   // optimize
   optimizeTape(classClassScope.tape);

   // create byte code
   classClassScope.save(_writer);
}

void Compiler :: compileClassDeclaration(DNode node, ClassScope& scope, DNode hints)
{
   scope.compileClassHints(hints);

   _writer.declareClass(scope.tape, scope.reference);

   DNode member = node.firstChild();
   if (member==nsBaseClass) {
      compileParentDeclaration(member, scope);

      member = member.nextNode();
   }
   else compileParentDeclaration(DNode(), scope);

   compileFieldDeclarations(member, scope);

   // check if the class is stateless
   if (scope.info.fields.Count() == 0
      && !test(scope.info.header.flags, elStructureRole)
      && !test(scope.info.header.flags, elDynamicRole)
      /* && !test(scope.info.header.flags, elWithLocker)*/)
   {
      scope.info.header.flags |= elStateless;

      scope.moduleScope->defineConstantSymbol(scope.reference);
   }
   else scope.info.header.flags &= ~elStateless;

   // if type size is provided make sure the class is compatible
   if (scope.info.header.typeRef && scope.moduleScope->sizeHints.exist(scope.info.header.typeRef)) {
      if (scope.info.size < scope.moduleScope->sizeHints.get(scope.info.header.typeRef))
         scope.raiseError(errNotSupprotedType, node.Terminal());
   }

   // if type wrapper is proveded make sure the current code is the wrapper
   if (scope.info.header.typeRef && scope.moduleScope->typeHints.get(scope.info.header.typeRef) != 0) {
      if (scope.reference != scope.moduleScope->typeHints.get(scope.info.header.typeRef))
         scope.raiseError(errNotSupprotedType, node.Terminal());
   }

   // if it is super class or a role
   if (scope.info.header.parentRef == 0 || test(scope.info.header.flags, elRole)) {
      // super class is class class
      scope.info.classClassRef = scope.reference;
   }
   else {
      // define class class name
      IdentifierString classClassName(scope.moduleScope->module->resolveReference(scope.reference));
      classClassName.append(CLASSCLASS_POSTFIX);

      scope.info.classClassRef = scope.moduleScope->module->mapReference(classClassName);
   }

   compileVMT(member, scope);

   // add default handlers for base class
   if (scope.info.header.parentRef == 0) {
      compileImportMethod(node, scope, encodeVerb(DISPATCH_MESSAGE_ID), DISPATCH_ROUTINE);

      // NOTE: due to the current implementation - there are two dispatch handler (second one - generic type dispatch)
      compileImportMethod(node, scope, encodeVerb(GENERIC_MESSAGE_ID), TRY_DISPATCH_ROUTINE);
   }

   _writer.endClass(scope.tape);

   // compile explicit symbol
   compileSymbolCode(scope);

   // optimize
   optimizeTape(scope.tape);

   // create byte code
   scope.save(_writer);
}

void Compiler :: compileSymbolDeclaration(DNode node, SymbolScope& scope, DNode hints, bool isStatic)
{
   bool constant = false;
   scope.compileHints(hints, constant);

   DNode expression = node.firstChild();

   // compile symbol into byte codes
   if (isStatic) {
      _writer.declareStaticSymbol(scope.tape, scope.reference);
   }
   else _writer.declareSymbol(scope.tape, scope.reference);

   CodeScope codeScope(&scope);

   DNode importBody = node.select(nsImport);
   // check if it is external code
   if (importBody == nsImport) {
      ReferenceNs reference(PACKAGE_MODULE, INLINE_MODULE);
      reference.combine(importBody.Terminal());

      importCode(importBody, *scope.moduleScope, codeScope.tape, reference);
   }
   else {
      // compile symbol body
      ObjectInfo retVal = compileExpression(expression, codeScope, HINT_ROOT);
      _writer.loadObject(*codeScope.tape, retVal);

      // create constant if required
      if (constant && (retVal.kind == okIntConstant || retVal.kind == okLiteralConstant)) {
         _Module* module = scope.moduleScope->module;

         MemoryWriter dataWriter(module->mapSection(scope.reference | mskRDataRef, false));
         if (retVal.kind == okIntConstant) {
            int value = StringHelper::strToInt(module->resolveConstant(retVal.param));

            dataWriter.writeDWord(value);

            dataWriter.Memory()->addReference(scope.moduleScope->mapConstantReference(INT_CLASS) | mskVMTRef, -4);

            scope.moduleScope->defineIntConstant(scope.reference);
         }
         else if (retVal.kind == okLiteralConstant) {
            const wchar16_t* value = module->resolveConstant(retVal.param);

            dataWriter.writeWideLiteral(value, getlength(value) + 1);

            dataWriter.Memory()->addReference(scope.moduleScope->mapConstantReference(WSTR_CLASS) | mskVMTRef, -4);

             scope.moduleScope->defineLiteralConstant(scope.reference);
         }
      }
   }

   if (isStatic) {
      // HOTFIX : contains no symbol ending tag, to correctly place an expression end debug symbol
      _writer.exitStaticSymbol(scope.tape, scope.reference);
   }

   _writer.declareBreakpoint(scope.tape, 0, 0, 0, dsVirtualEnd);

   _writer.endSymbol(scope.tape);

   // optimize
   optimizeTape(scope.tape);

   // create byte code sections
   _writer.compile(scope.tape, scope.moduleScope->module, scope.moduleScope->debugModule, scope.moduleScope->sourcePathRef);
}

void Compiler :: compileIncludeModule(DNode node, ModuleScope& scope, DNode hints)
{
   if (hints != nsNone)
      scope.raiseWarning(wrnUnknownHint, hints.Terminal());

   TerminalInfo ns = node.Terminal();

   // check if the module exists
   _Module* module = scope.project->loadModule(ns, true);
   if (!module)   
      scope.raiseWarning(wrnUnknownModule, ns);

   const wchar16_t* value = retrieve(scope.defaultNs.start(), ns, NULL);
   if (value == NULL) {
      scope.defaultNs.add(ns.value);

      // load extensions
      scope.loadExtensions(ns, module);
   }
}

void Compiler :: compileForward(DNode node, ModuleScope& scope, DNode hints)
{
   bool constant;
   scope.compileForwardHints(hints, constant);

   TerminalInfo shortcut = node.Terminal();

   if (!scope.defineForward(shortcut.value, node.firstChild().Terminal().value, constant))
      scope.raiseError(errDuplicatedDefinition, shortcut);
}

void Compiler :: compileType(DNode& member, ModuleScope& scope, DNode hints)
{
   if (!ConstantIdentifier::compare(scope.module->Name(), STANDARD_MODULE, 6))
      scope.raiseError(errNotApplicable, member.Terminal());

   ref_t typeRef = scope.mapSubject(member.Terminal());

   int size = 0;
   ref_t wrapper = 0;
   ref_t synonymRef = 0;
   while (hints == nsHint) {
      TerminalInfo terminal = hints.Terminal();

      if (ConstantIdentifier::compare(terminal, HINT_SIZE)) {
         TerminalInfo sizeValue = hints.select(nsHintValue).Terminal();
         if (sizeValue.symbol == tsInteger) {
            size = StringHelper::strToInt(sizeValue.value);
         }
         else scope.raiseWarning(wrnUnknownHint, terminal);         
      }
      else if (ConstantIdentifier::compare(terminal, HINT_ITEMSIZE)) {
         TerminalInfo sizeValue = hints.select(nsHintValue).Terminal();
         if (sizeValue.symbol == tsInteger) {
            size = -StringHelper::strToInt(sizeValue.value);
         }
         else scope.raiseWarning(wrnUnknownHint, terminal);         
      }
      else if (ConstantIdentifier::compare(terminal, HINT_WRAPPER)) {
         TerminalInfo roleValue = hints.select(nsHintValue).Terminal();
         wrapper = scope.mapTerminal(roleValue);

         scope.validateReference(roleValue, wrapper);
      }
      else if (ConstantIdentifier::compare(terminal, HINT_SYNONYM)) {
         TerminalInfo typeValue = hints.select(nsHintValue).Terminal();

         synonymRef = typeRef;
         typeRef = scope.mapType(typeValue);
         if (typeRef == 0)
            scope.raiseError(errUnknownSubject, typeValue);
      }
      else scope.raiseWarning(wrnUnknownHint, hints.Terminal());

      hints = hints.nextNode();
   }

   if (synonymRef != 0) {
      if (size != 0 || wrapper != 0)
         scope.raiseWarning(wrnInvalidHint, hints.Terminal());

      bool valid = scope.saveSynonym(typeRef, synonymRef);

      if (!valid)
         scope.raiseError(errDuplicatedDefinition, member.Terminal());
   }
   else {
      bool valid = scope.saveType(typeRef, wrapper, size);

      if (!valid)
         scope.raiseError(errDuplicatedDefinition, member.Terminal());
   }
}

void Compiler :: compileDeclarations(DNode& member, ModuleScope& scope)
{
   while (member != nsNone) {
      DNode hints = skipHints(member);

      TerminalInfo name = member.Terminal();

      switch (member) {
         case nsType:
            compileType(member, scope, hints);
            break;
         case nsClass:
         {
            ref_t reference = scope.mapTerminal(name);

            // check for duplicate declaration
            if (scope.module->mapSection(reference | mskSymbolRef, true))
               scope.raiseError(errDuplicatedSymbol, name);

            // compile class
            ClassScope classScope(&scope, reference);
            compileClassDeclaration(member, classScope, hints);

            // compile class class if it is not a super class or a role
            if (classScope.info.classClassRef != classScope.reference && !test(classScope.info.header.flags, elRole)) {
               ClassScope classClassScope(&scope, classScope.info.classClassRef);
               compileClassClassDeclaration(member, classClassScope, classScope);
            }
            break;
         }
         case nsSymbol:
         case nsStatic:
         {
            ref_t reference = scope.mapTerminal(name);

            // check for duplicate declaration
            if (scope.module->mapSection(reference | mskSymbolRef, true))
               scope.raiseError(errDuplicatedSymbol, name);

            SymbolScope symbolScope(&scope, reference);
            compileSymbolDeclaration(member, symbolScope, hints, (member == nsStatic));
            break;
         }
      }
      member = member.nextNode();
   }
}

void Compiler :: compileIncludeSection(DNode& member, ModuleScope& scope)
{
   while (member != nsNone) {
      DNode hints = skipHints(member);

      switch (member) {
         case nsInclude:
            if (member.firstChild() == nsForward) {
               compileForward(member, scope, hints);
            }
            else compileIncludeModule(member, scope, hints);
            break;
         default:
            // due to current syntax we need to reset hints back, otherwise they will be skipped
            if (hints != nsNone)
               member = hints;

            return;
      }
      member = member.nextNode();
   }
}

void Compiler :: compileModule(DNode node, ModuleScope& scope)
{
   DNode member = node.firstChild();

   compileIncludeSection(member, scope);

   compileDeclarations(member, scope);
}

bool Compiler :: validate(Project& project, _Module* module, int reference)
{
   int   mask = reference & mskAnyRef;
   ref_t extReference = 0;
   const wchar16_t* refName = module->resolveReference(reference & ~mskAnyRef);
   _Module* extModule = project.resolveModule(refName, extReference, true);

   return (extModule != NULL && extModule->mapSection(extReference | mask, true) != NULL);
}

void Compiler :: validateUnresolved(Unresolveds& unresolveds, Project& project)
{
   for (List<Unresolved>::Iterator it = unresolveds.start() ; !it.Eof() ; it++) {
      if (!validate(project, (*it).module, (*it).reference)) {
         const wchar16_t* refName = (*it).module->resolveReference((*it).reference & ~mskAnyRef);

         project.raiseWarning(wrnUnresovableLink, (*it).fileName, (*it).row, (*it).col, refName);
      }
   }
}

void Compiler :: compile(const tchar_t* source, MemoryDump* buffer, ModuleScope& scope)
{
   scope.sourcePathRef = _writer.writeSourcePath(scope.debugModule, scope.sourcePath);

   // parse
   TextFileReader sourceFile(source, scope.project->getDefaultEncoding(), true);
   if (!sourceFile.isOpened())
      scope.project->raiseError(errInvalidFile, source);

   buffer->clear();
   MemoryWriter bufWriter(buffer);
   DerivationWriter writer(&bufWriter);
   _parser.parse(&sourceFile, &writer, scope.project->getTabSize());

   // compile
   MemoryReader bufReader(buffer);
   DerivationReader reader(&bufReader);

   compileModule(reader.readRoot(), scope);
}

void Compiler :: createModuleInfo(ModuleScope& scope, const tchar_t* path, bool withDebugInfo, Map<const wchar16_t*, ModuleInfo>& modules)
{
   _Module* module = scope.project->createModule(path);

   const wchar16_t* name = module->Name();

   ModuleInfo info = modules.get(name);
   if (!info.codeModule) {
      info.codeModule = module;
      if (withDebugInfo)
         info.debugModule = scope.project->createDebugModule(name);

      modules.add(name, info);
   }

   scope.init(module, info.debugModule);
}

bool Compiler :: run(Project& project)
{
   bool withDebugInfo = project.BoolSetting(opDebugMode);
   Map<const wchar16_t*, ModuleInfo> modules(ModuleInfo(NULL, NULL));

   MemoryDump  buffer;                // temporal derivation buffer
   Unresolveds unresolveds(Unresolved(), NULL);
   for(SourceIterator it = project.getSourceIt() ; !it.Eof() ; it++) {
      try {
         // create or update module
         ModuleScope scope(&project, it.key(), &unresolveds);
         createModuleInfo(scope, it.key(), withDebugInfo, modules);

         project.printInfo("%s", scope.sourcePath);

         // compile source
         compile(*it, &buffer, scope);
      }
      catch (LineTooLong& e)
      {
         project.raiseError(errLineTooLong, it.key(), e.row, 1);
      }
      catch (InvalidChar& e)
      {
         project.raiseError(errInvalidChar, it.key(), e.row, e.column, String<wchar16_t, 2>(&e.ch, 1));
      }
      catch (SyntaxError& e)
      {
         project.raiseError(e.error, it.key(), e.row, e.column, e.token);
      }
   }

   Map<const wchar16_t*, ModuleInfo>::Iterator it = modules.start();
   while (!it.Eof()) {
      ModuleInfo info = *it;

      project.saveModule(info.codeModule, _T("nl"));

      if (info.debugModule)
         project.saveModule(info.debugModule, _T("dnl"));

      it++;
   }

   // validate the unresolved forward refereces if unresolved reference warning is enabled
   validateUnresolved(unresolveds, project);

   return !project.HasWarnings();
}
