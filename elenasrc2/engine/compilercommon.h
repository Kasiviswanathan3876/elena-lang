//---------------------------------------------------------------------------
//		E L E N A   P r o j e c t:  ELENA Compiler
//
//		This file contains ELENA compiler common interfaces.
//
//                                              (C)2005-2019, by Alexei Rakov
//---------------------------------------------------------------------------

#ifndef compilerCommonH
#define compilerCommonH

#include "elena.h"
#include "syntaxtree.h"
#include "bytecode.h"

constexpr auto V_CATEGORY_MASK   = 0x7FFFFF00u;
constexpr auto V_CATEGORY_MAX    = 0x0000F000u;

// attributes / prmitive types

/// visibility:
constexpr auto V_PUBLIC          = 0x80005001u;
constexpr auto V_PRIVATE         = 0x80005002u;
constexpr auto V_INTERNAL        = 0x80005003u;

/// property:
constexpr auto V_SEALED          = 0x80004001u;
constexpr auto V_ABSTRACT        = 0x80004002u;
constexpr auto V_CLOSED          = 0x80004003u;
constexpr auto V_PREDEFINED      = 0x80004005u;

/// accessors:
constexpr auto V_GETACCESSOR = 0x80003007u;
constexpr auto V_SETACCESSOR = 0x80003008u;

/// scope_prefix:
constexpr auto V_CONST           = 0x80002001u;
constexpr auto V_EMBEDDABLE      = 0x80002002u;
constexpr auto V_WRAPPER         = 0x80002003u;
constexpr auto V_DIRECT          = 0x80002004u;
constexpr auto V_LOOP            = 0x80002005u;
constexpr auto V_PRELOADED       = 0x80002006u;
constexpr auto V_LAZY            = 0x80002009u;

/// scope:
constexpr auto V_CLASS           = 0x80001001u;
constexpr auto V_STRUCT          = 0x80001002u;
constexpr auto V_SYMBOLEXPR      = 0x80001003u;
constexpr auto V_CONSTRUCTOR     = 0x80001004u;
constexpr auto V_EXTENSION       = 0x80001005u;
constexpr auto V_SINGLETON       = 0x80001006u;
constexpr auto V_LIMITED         = 0x80001007u;
constexpr auto V_METHOD          = 0x80001008u;
constexpr auto V_FIELD           = 0x80001009u;
constexpr auto V_TYPETEMPL       = 0x8000100Au;
constexpr auto V_GENERIC         = 0x8000100Bu;
constexpr auto V_ACTION          = 0x8000100Cu;     // a closure attribute
constexpr auto V_VARIABLE        = 0x8000100Du;
constexpr auto V_MEMBER          = 0x8000100Eu;
constexpr auto V_STATIC          = 0x8000100Fu;
constexpr auto V_CONVERSION      = 0x80001011u;
constexpr auto V_NEWOP           = 0x80001012u;
constexpr auto V_DISPATCHER      = 0x80001013u;
constexpr auto V_ARGARRAY        = 0x80001014u;
constexpr auto V_EXTERN          = 0x80001015u;
constexpr auto V_INTERN          = 0x80001016u;
constexpr auto V_FORWARD         = 0x80001017u;
constexpr auto V_IMPORT          = 0x80001018u;
constexpr auto V_GROUP           = 0x80001019u;
constexpr auto V_PROPERTY        = 0x8000101Au;
constexpr auto V_NOSTRUCT        = 0x8000101Bu;
constexpr auto V_AUTO            = 0x8000101Cu;
constexpr auto V_INITIALIZER     = 0x8000101Du;
constexpr auto V_TEMPLATE        = 0x8000101Eu;

/// primitive type attributes
constexpr auto V_STRING          = 0x80000801u;
constexpr auto V_FLOAT           = 0x80000802u;
constexpr auto V_INTBINARY       = 0x80000803u;
constexpr auto V_BINARY          = 0x80000804u;
constexpr auto V_PTRBINARY       = 0x80000805u;
constexpr auto V_MESSAGE         = 0x80000806u;
constexpr auto V_SUBJECT         = 0x80000807u;
constexpr auto V_SYMBOL          = 0x80000808u;

/// primitive types
constexpr auto V_FLAG            = 0x80000001u;
constexpr auto V_NIL             = 0x80000002u;
constexpr auto V_INT32           = 0x80000003u;
constexpr auto V_INT64           = 0x80000004u;
constexpr auto V_DWORD           = 0x80000005u;
constexpr auto V_REAL64          = 0x80000006u;
constexpr auto V_EXTMESSAGE      = 0x80000007u;
constexpr auto V_PTR32           = 0x80000008u;
constexpr auto V_OBJARRAY        = 0x80000009u;
constexpr auto V_INT32ARRAY      = 0x8000000Au;
constexpr auto V_BINARYARRAY     = 0x8000000Bu;
constexpr auto V_INT16ARRAY      = 0x8000000Cu;
constexpr auto V_INT8ARRAY       = 0x8000000Du;
constexpr auto V_OBJECT          = 0x8000000Eu;
constexpr auto V_UNBOXEDARGS     = 0x8000000Fu;

//#define V_PARAMETER      (ref_t)-02
constexpr auto V_STRCONSTANT     = 0x80000010u; // used for explicit constant operations
//#define V_TAPEGROUP      (ref_t)-8209

namespace _ELENA_
{

typedef Map<ident_t, ref_t>      ForwardMap;

enum MethodHint
{
   tpMask        = 0x00000F,

   tpUnknown     = 0x000000,
   tpSealed      = 0x000001,
   tpClosed      = 0x000002,
   tpNormal      = 0x000003,
   tpDispatcher  = 0x000004,
   tpPrivate     = 0x000005,
   tpStackSafe   = 0x000010,
   tpEmbeddable  = 0x000020,
   tpGeneric     = 0x000040,
   tpAction      = 0x000080,
   tpConstructor = 0x200400,
   tpConversion  = 0x200800,
   tpMultimethod = 0x001000,
   tpStatic      = 0x004000,
   tpGetAccessor = 0x008000,
   tpSpecial     = 0x010000,
   tpAbstract    = 0x020000,
   tpInternal    = 0x040000,
   tpPredefined  = 0x080000, // virtual class declaration
   tpDynamic     = 0x100000, // indicates that the method does not accept stack allocated parameters
   tpInitializer = 0x200000,
   tpSetAccessor = 0x400000,
};

// --- _Project ---

class _ProjectManager
{
public:
//   virtual ident_t Namespace() const = 0;
//
//   virtual int getDefaultEncoding() = 0; // !! obsolete!?
//   virtual int getTabSize() = 0; // !! obsolete!?
//
//   //   virtual bool HasWarnings() const = 0;     // !! obsolete
//   //   virtual bool WarnOnWeakUnresolved() const = 0;

   virtual ident_t getManinfestName() = 0;
   virtual ident_t getManinfestVersion() = 0;
   virtual ident_t getManinfestAuthor() = 0;

   virtual void printInfo(const char* msg, ident_t value) = 0;

   virtual void raiseError(ident_t msg) = 0;
   virtual void raiseError(ident_t msg, ident_t path, int row, int column, ident_t terminal = NULL) = 0;
   virtual void raiseError(ident_t msg, ident_t value) = 0;

//   virtual void raiseErrorIf(bool throwExecption, ident_t msg, ident_t identifier) = 0;

   virtual void raiseWarning(int level, ident_t msg, ident_t path, int row, int column, ident_t terminal = NULL) = 0;
//   virtual void raiseWarning(int level, ident_t msg, ident_t path) = 0;

   virtual _Module* createModule(ident_t name) = 0;
   virtual _Module* createDebugModule(ident_t name) = 0;

   virtual _Module* loadModule(ident_t package, bool silentMode) = 0;
//   //   virtual void saveModule(_Module* module, ident_t extension) = 0; // !! obsolete

   virtual _Module* resolveModule(ident_t referenceName, ref_t& reference, bool silentMode = false) = 0;
   virtual _Module* resolveWeakModule(ident_t weakReferenceName, ref_t& reference, bool silentMode = false) = 0;

   virtual ident_t resolveForward(ident_t forward) = 0;

   virtual bool addForward(ident_t forward, ident_t reference) = 0;

   virtual ident_t resolveExternalAlias(ident_t alias, bool& stdCall) = 0;
};

// class forward declaration
class _Compiler;

// --- _ModuleScope ---

struct _ModuleScope
{
   struct BranchingInfo
   {
      ref_t reference;
      ref_t trueRef;
      ref_t falseRef;

      BranchingInfo()
      {
         reference = 0;
         trueRef = falseRef = 0;
      }
   };

   _ProjectManager*  project;

   _Module*          module;
   _Module*          debugModule;

   // cached references
   ref_t             superReference;
   ref_t             intReference;
   ref_t             longReference;
   ref_t             realReference;
   ref_t             messageNameReference;
   ref_t             messageReference;
   ref_t             extMessageReference;
   ref_t             literalReference;
   ref_t             wideReference;
   ref_t             charReference;
   ref_t             refTemplateReference;
   ref_t             arrayTemplateReference;
   ref_t             argArrayTemplateReference;
   ref_t             closureTemplateReference;
   ref_t             lazyExprReference;
   ref_t             wrapReference;

   // cached messages
   ref_t             dispatch_message;
   ref_t             newobject_message;
   ref_t             init_message;
   ref_t             constructor_message;

   // cached bool values
   BranchingInfo     branchingInfo;

   // cached paths
   SymbolMap         savedPaths;

   MessageMap        attributes;

   virtual ref_t mapAnonymous(ident_t prefix = nullptr) = 0;

////   virtual ref_t mapAttribute(SNode terminal) = 0;
////   virtual ref_t mapTerminal(SNode terminal, bool existing = false) = 0;
//
//   virtual SubjectList* getAutogerenatedExtensions(ref_t attr) = 0;
   virtual void saveAttribute(ident_t typeName, ref_t classReference) = 0;
//   virtual void saveAutogerenatedExtension(ref_t attr, ref_t extension) = 0;

   virtual ref_t loadClassInfo(ClassInfo& info, ref_t reference, bool headerOnly = false) = 0;
   virtual ref_t loadClassInfo(ClassInfo& info, ident_t vmtName, bool headerOnly = false) = 0;
   virtual ref_t loadSymbolExpressionInfo(SymbolExpressionInfo& info, ident_t symbolName) = 0;

   virtual _Module* loadReferenceModule(ident_t referenceName, ref_t& reference) = 0;
   virtual _Module* loadReferenceModule(ref_t reference, ref_t& moduleReference) = 0;

   bool isClassDeclared(ref_t reference)
   {
      if (!reference) {
         return false;
      }
      else return mapSection(reference | mskMetaRDataRef, true) != nullptr;
   }

   virtual _Memory* mapSection(ref_t reference, bool existing) = 0;
   virtual ref_t mapTemplateClass(ident_t ns, ident_t templateName, bool& alreadyDeclared) = 0;

   virtual void importClassInfo(ClassInfo& copy, ClassInfo& target, _Module* exporter, bool headerOnly, bool inheritMode, 
                                 bool ignoreFields) = 0;

   virtual ref_t resolveClosure(ref_t closureMessage, ref_t outputRef, ident_t ns) = 0;

   virtual ref_t mapNewIdentifier(ident_t ns, ident_t identifier, bool privateOne) = 0;
   virtual ref_t mapFullReference(ident_t referenceName, bool existing = false) = 0;

   virtual ref_t resolveImplicitIdentifier(ident_t ns, ident_t identifier, bool referenceOne, IdentifierList* importedNs) = 0;
   virtual ident_t resolveFullName(ref_t reference) = 0;
   virtual ident_t resolveFullName(ident_t referenceName) = 0;

   void raiseError(const char* message, ident_t sourcePath, SNode node)
   {
      SNode terminal = SyntaxTree::findTerminalInfo(node);

      int col = terminal.findChild(lxCol).argument;
      int row = terminal.findChild(lxRow).argument;
      ident_t identifier = terminal.identifier();
      if (emptystr(identifier))
         identifier = terminal.identifier();

      project->raiseError(message, sourcePath, row, col, identifier);
   }

   void raiseWarning(int level, const char* message, ident_t sourcePath, SNode node)
   {
      SNode terminal = SyntaxTree::findTerminalInfo(node);

      int col = terminal.findChild(lxCol).argument;
      int row = terminal.findChild(lxRow).argument;
      ident_t identifier = terminal.identifier();

      project->raiseWarning(level, message, sourcePath, row, col, identifier);
   }

   void printMessageInfo(const char* info, ref_t message)
   {
      IdentifierString messageName;
      ByteCodeCompiler::resolveMessageName(messageName, module, message);

      project->printInfo(info, messageName.ident());
   }

//   void raiseWarning(int level, const char* message, ident_t sourcePath, ident_t identifier)
//   {
//      project->raiseWarning(level, message, sourcePath, 0, 0, identifier);
//   }
//
//   void raiseWarning(int level, const char* message, ident_t sourcePath)
//   {
//      project->raiseWarning(level, message, sourcePath);
//   }

   virtual ref_t generateTemplate(ref_t reference, List<SNode>& parameters, ident_t ns, bool declarationMode) = 0;
   virtual void generateTemplateCode(SyntaxWriter& writer, ref_t reference, List<SNode>& parameters) = 0;
   virtual void generateTemplateProperty(SyntaxWriter& writer, ref_t reference, List<SNode>& parameters) = 0;
   virtual void generateExtensionTemplate(SyntaxTree& tree, ident_t ns, ref_t extensionRef) = 0;
   virtual void importClassTemplate(SyntaxWriter& writer, ref_t reference, List<SNode>& parameters) = 0;

   virtual bool includeNamespace(IdentifierList& importedNs, ident_t name, bool& duplicateInclusion) = 0;

   _ModuleScope()
      : attributes(0), savedPaths(-1)
   {
      project = nullptr;
      debugModule = module = nullptr;
      intReference = superReference = 0;
      messageNameReference = messageReference = 0;
      longReference = literalReference = wideReference = 0;
      charReference = realReference = 0;
      closureTemplateReference = refTemplateReference = 0;
      lazyExprReference = extMessageReference = 0;
      arrayTemplateReference = 0;
      wrapReference = argArrayTemplateReference = 0;

      init_message = newobject_message = dispatch_message = 0;
      constructor_message = 0;
   }
};

// --- _Compiler ---

class _Compiler
{
public:
   virtual ref_t resolvePrimitiveReference(_ModuleScope& scope, ref_t argRef, ref_t elementRef, ident_t ns, bool declarationMode) = 0;

   virtual void injectBoxing(SyntaxWriter& writer, _ModuleScope& scope, LexicalType boxingType, int argument, ref_t targetClassRef, bool arrayMode = false) = 0;
   virtual void injectConverting(SyntaxWriter& writer, LexicalType convertOp, int convertArg, LexicalType targetOp, int targetArg, ref_t targetClassRef/*,
      ref_t targetRef*/, int stacksafeAttr) = 0;
////   virtual void injectFieldExpression(SyntaxWriter& writer) = 0;

   virtual void injectEmbeddableGet(SNode assignNode, SNode callNode, ref_t subject) = 0;
   virtual void injectEmbeddableOp(_ModuleScope& scope, SNode assignNode, SNode callNode, ref_t subject, int paramCount/*, int verb*/) = 0;
   virtual void injectEmbeddableConstructor(SNode classNode, ref_t message, ref_t privateRef/*, ref_t genericMessage*/) = 0;
   virtual void injectVirtualMultimethod(_ModuleScope& scope, SNode classNode, ref_t message, LexicalType methodType) = 0;
   virtual void injectVirtualMultimethodConversion(_ModuleScope& scope, SNode classNode, ref_t message, LexicalType methodType) = 0;
//   virtual void injectVirtualArgDispatcher(_CompilerScope& scope, SNode classNode, ref_t message, LexicalType methodType) = 0;
   virtual void injectVirtualReturningMethod(_ModuleScope& scope, SNode classNode, ref_t message, ident_t variable, ref_t outputRef) = 0;
   virtual void injectVirtualDispatchMethod(SNode classNode, ref_t message, LexicalType type, ident_t argument) = 0;
//   virtual void injectDirectMethodCall(SyntaxWriter& writer, ref_t targetRef, ref_t message) = 0;
//
   virtual void injectLocalBoxing(SNode node, int size) = 0;
////   //virtual int injectTempLocal(SNode node) = 0;
//
//   virtual void injectVirtualStaticConstField(_CompilerScope& scope, SNode classNode, ident_t fieldName, ref_t fieldRef) = 0;
//
//   virtual void generateListMember(_CompilerScope& scope, ref_t enumRef, ref_t memberRef) = 0;
   virtual void generateOverloadListMember(_ModuleScope& scope, ref_t enumRef, ref_t memberRef) = 0;
   virtual void generateClosedOverloadListMember(_ModuleScope& scope, ref_t enumRef, ref_t memberRef, ref_t classRef) = 0;
   virtual void generateSealedOverloadListMember(_ModuleScope& scope, ref_t enumRef, ref_t memberRef, ref_t classRef) = 0;
   virtual ref_t generateExtensionTemplate(_ModuleScope& scope, ref_t templateRef, size_t argumentLen, ref_t* arguments, ident_t ns) = 0;

   virtual void registerExtensionTemplate(SyntaxTree& tree, _ModuleScope& scope, ident_t ns, ref_t extensionRef) = 0;

   virtual bool declareModule(SyntaxTree& tree, _ModuleScope& scope, bool forcedDeclaration, bool& repeatMode) = 0;
   virtual void compileModule(SyntaxTree& syntaxTree, _ModuleScope& scope, ident_t greeting) = 0;

////   virtual ref_t readEnumListMember(_CompilerScope& scope, _Module* extModule, MemoryReader& reader) = 0;
};

// --- _CompilerLogic ---

class _CompilerLogic
{
public:
   struct ExpressionAttributes
   {
      bool  typeAttr;
      bool  castAttr;
      bool  newOpAttr;
      bool  forwardAttr;
      bool  externAttr;
      bool  refAttr;
	   bool  paramsAttr;
	   bool  internAttr;
      bool  loopAttr;
      bool  memberAttr;
      bool  subjAttr;
      bool  mssgAttr;
      bool  wrapAttr;
      bool  classAttr;
      bool  directAttr;
      bool  lazyAttr;

      bool isExprAttr()
      {
         return paramsAttr | refAttr | internAttr | externAttr | forwardAttr | memberAttr | subjAttr | wrapAttr | mssgAttr | classAttr | directAttr | lazyAttr;
      } 

      ExpressionAttributes()
      {
         refAttr = externAttr = typeAttr = castAttr = forwardAttr = false;
         paramsAttr = newOpAttr = loopAttr = internAttr = false;
         classAttr = mssgAttr = wrapAttr = subjAttr = memberAttr = false;
         lazyAttr = directAttr = false;
      }
   };

   struct ChechMethodInfo
   {
      bool  found;
      bool  directResolved;
      bool  withCustomDispatcher;
      bool  stackSafe;
      bool  embeddable;
//      bool  withOpenArgDispatcher;
//      bool  withOpenArg1Dispatcher;
//      bool  withOpenArg2Dispatcher;
      bool  closure;
      bool  dynamicRequired;
      ref_t outputReference;

      ChechMethodInfo()
      {
         directResolved = false;
         embeddable = found = false;
         outputReference = 0;
         withCustomDispatcher = false;
         stackSafe = false;
//         withOpenArgDispatcher = false;
//         withOpenArg1Dispatcher = false;
//         withOpenArg2Dispatcher = false;
         closure = false;
         dynamicRequired = false;
      }
   };

   virtual int checkMethod(_ModuleScope& scope, ref_t reference, ref_t message, ChechMethodInfo& result) = 0;
   virtual int checkMethod(ClassInfo& info, ref_t message, ChechMethodInfo& result) = 0;

   // retrieve the class info / size
   virtual bool defineClassInfo(_ModuleScope& scope, ClassInfo& info, ref_t reference, bool headerOnly = false) = 0;

   virtual int defineStructSizeVariable(_ModuleScope& scope, ref_t reference, ref_t elementRef, bool& variable) = 0;
   virtual int defineStructSize(_ModuleScope& scope, ref_t reference, ref_t elementRef) = 0;
   virtual int defineStructSize(ClassInfo& info, bool& variable) = 0;

   virtual ref_t definePrimitiveArray(_ModuleScope& scope, ref_t elementRef, bool structOne) = 0;

   // retrieve the call type
   virtual int resolveCallType(_ModuleScope& scope, ref_t& classReference, ref_t message, ChechMethodInfo& result) = 0;

   // retrieve the operation type
   virtual int resolveOperationType(_ModuleScope& scope, int operatorId, ref_t loperand, ref_t roperand, ref_t& result) = 0;
   virtual int resolveOperationType(_ModuleScope& scope, int operatorId, ref_t loperand, ref_t roperand, ref_t roperand2, ref_t& result) = 0;
   virtual int resolveNewOperationType(_ModuleScope& scope, ref_t loperand, ref_t roperand) = 0;

   // retrieve the branching operation type
   virtual bool resolveBranchOperation(_ModuleScope& scope, int operatorId, ref_t loperand, ref_t& reference) = 0;

   virtual ref_t retrievePrimitiveReference(_ModuleScope& scope, ClassInfo& info) = 0;

   // check if the classes is compatible
   virtual bool isCompatible(_ModuleScope& scope, ref_t targetRef, ref_t sourceRef) = 0;

   virtual bool isVariable(_ModuleScope& scope, ref_t targetRef) = 0;
   virtual bool isValidType(_ModuleScope& scope, ref_t targetRef, bool ignoreUndeclared) = 0;
   virtual bool isArray(_ModuleScope& scope, ref_t targetRef) = 0;

   virtual bool isWrapper(ClassInfo& info) = 0;
   virtual ref_t resolvePrimitive(ClassInfo& info, ref_t& element) = 0;
   // check if the class can be used as a fixed-size embeddable array
   virtual bool isEmbeddableArray(ClassInfo& info) = 0;
   virtual bool isVariable(ClassInfo& info) = 0;
   virtual bool isEmbeddable(ClassInfo& info) = 0;
   virtual bool isEmbeddable(_ModuleScope& scope, ref_t reference) = 0;
//   virtual bool isMethodStacksafe(ClassInfo& info, ref_t message) = 0;
   virtual bool isMethodAbstract(ClassInfo& info, ref_t message) = 0;
   virtual bool isMethodGeneric(ClassInfo& info, ref_t message) = 0;
   virtual bool isMultiMethod(ClassInfo& info, ref_t message) = 0;
   virtual bool isClosure(ClassInfo& info, ref_t message) = 0;
//   virtual bool isDispatcher(ClassInfo& info, ref_t message) = 0;   

   // class is considered to be a role if it cannot be initiated
   virtual bool isRole(ClassInfo& info) = 0;          
   virtual bool isAbstract(ClassInfo& info) = 0;
   virtual bool validateAutoType(_ModuleScope& scope, ref_t& reference) = 0;

   virtual bool isWithEmbeddableDispatcher(_ModuleScope& scope, SNode node) = 0;

   // auto generate virtual methods / fields
   virtual void injectVirtualCode(_ModuleScope& scope, SNode node, ref_t classRef, ClassInfo& info, _Compiler& compiler, bool closed) = 0;
//   virtual void injectVirtualFields(_CompilerScope& scope, SNode node, ref_t classRef, ClassInfo& info, _Compiler& compiler) = 0;
   virtual void injectVirtualMultimethods(_ModuleScope& scope, SNode node, _Compiler& compiler, List<ref_t>& implicitMultimethods, LexicalType methodType) = 0;
   virtual void verifyMultimethods(_ModuleScope& scope, SNode node, ClassInfo& info, List<ref_t>& implicitMultimethods) = 0;
   virtual void injectOperation(SyntaxWriter& writer, _ModuleScope& scope, int operatorId, int operation, ref_t& reference, ref_t elementRef) = 0;
   virtual bool injectImplicitConversion(SyntaxWriter& writer, _ModuleScope& scope, _Compiler& compiler, ref_t targetRef, ref_t sourceRef, 
      ref_t elementRef, ident_t ns) = 0;
   virtual ref_t resolveImplicitConstructor(_ModuleScope& scope, ref_t targetRef, ref_t signRef, int paramCount, int& stackSafeAttr) = 0;
   virtual void injectNewOperation(SyntaxWriter& writer, _ModuleScope& scope, int operation, ref_t targetRef, ref_t elementRef) = 0;
   virtual void injectInterfaceDisaptch(_ModuleScope& scope, _Compiler& compiler, SNode node, ref_t parentRef) = 0;
   virtual bool injectConstantConstructor(SyntaxWriter& writer, _ModuleScope& scope, _Compiler& compiler, ref_t targetRef, ref_t messageRef) = 0;

   // auto generate class flags
   virtual void tweakClassFlags(_ModuleScope& scope, _Compiler& compiler, ref_t classRef, ClassInfo& info, bool classClassMode) = 0;
   virtual void tweakPrimitiveClassFlags(ref_t classRef, ClassInfo& info) = 0;

   virtual void validateClassDeclaration(_ModuleScope& scope, ClassInfo& info, bool& withAbstractMethods, 
      bool& disptacherNotAllowed, bool& emptyStructure) = 0;

   // attribute validations
   virtual bool validateClassAttribute(int& attrValue) = 0;
   virtual bool validateMethodAttribute(int& attrValue, bool& explicitMode) = 0;
   virtual bool validateImplicitMethodAttribute(int& attrValue) = 0;
   virtual bool validateFieldAttribute(int& attrValue, bool& isSealed, bool& isConstant, bool& isEmbeddable) = 0;
   virtual bool validateExpressionAttribute(ref_t attrValue, ExpressionAttributes& attributes) = 0;
   virtual bool validateSymbolAttribute(int attrValue, bool& constant, bool& staticOne, bool& preloadedOne) = 0;
   virtual bool validateMessage(_ModuleScope& scope, ref_t message, bool isClassClass) = 0;
   virtual bool validateArgumentAttribute(int attrValue, bool& byRefArg, bool& paramsArg) = 0;

   virtual bool isDefaultConstructorEnabled(ClassInfo& info) = 0;

   // optimization
   virtual bool validateBoxing(_ModuleScope& scope, _Compiler& compiler, SNode& node, ref_t targetRef, ref_t sourceRef, bool unboxingExpected, bool dynamicRequired) = 0;
   virtual bool recognizeEmbeddableGet(_ModuleScope& scope, SNode node, /*ref_t extensionRef, */ref_t returningRef, ref_t& actionRef) = 0;
//   virtual bool recognizeEmbeddableGetAt(_CompilerScope& scope, SNode node, ref_t extensionRef, ref_t returningRef, ref_t& subject) = 0;
//   virtual bool recognizeEmbeddableGetAt2(_CompilerScope& scope, SNode node, ref_t extensionRef, ref_t returningRef, ref_t& subject) = 0;
//   virtual bool recognizeEmbeddableEval(_CompilerScope& scope, SNode node, ref_t extensionRef, ref_t returningRef, ref_t& subject) = 0;
//   virtual bool recognizeEmbeddableEval2(_CompilerScope& scope, SNode root, ref_t extensionRef, ref_t returningRef, ref_t& subject) = 0;
   virtual bool recognizeEmbeddableIdle(SNode node, bool extensionOne) = 0;
   virtual bool recognizeEmbeddableMessageCall(SNode node, ref_t& messageRef) = 0;
   virtual bool optimizeEmbeddable(SNode node, _ModuleScope& scope) = 0;

   virtual bool optimizeEmbeddableGet(_ModuleScope& scope, _Compiler& compiler, SNode node) = 0;
   virtual bool optimizeEmbeddableOp(_ModuleScope& scope, _Compiler& compiler, SNode node) = 0;
   virtual void optimizeBranchingOp(_ModuleScope& scope, SNode node) = 0;

   virtual ref_t resolveMultimethod(_ModuleScope& scope, ref_t multiMessage, ref_t targetRef, ref_t implicitSignatureRef, int& stackSafeAttr) = 0;
   virtual ref_t resolveExtensionTemplate(_ModuleScope& scope, _Compiler& compiler, ident_t pattern, ref_t signatureRef, ident_t ns) = 0;
};

}  // _ELENA_

#endif // compilerCommonH
