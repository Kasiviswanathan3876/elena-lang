//---------------------------------------------------------------------------
//		E L E N A   P r o j e c t:  ELENA Compiler
//
//		This file contains ELENA compiler logic class implementation.
//
//                                              (C)2005-2018, by Alexei Rakov
//---------------------------------------------------------------------------

#include "elena.h"
// --------------------------------------------------------------------------
#include "compilerlogic.h"
#include "errors.h"

using namespace _ELENA_;

typedef ClassInfo::Attribute Attribute;

inline ref_t firstNonZero(ref_t ref1, ref_t ref2)
{
   return ref1 ? ref1 : ref2;
}

inline bool isWrappable(int flags)
{
   return !test(flags, elWrapper) && test(flags, elSealed);
}

inline bool isPrimitiveArrayRef(ref_t classRef)
{
   return classRef == V_OBJARRAY;
}

inline bool isOpenArgRef(ref_t classRef)
{
   return classRef == V_ARGARRAY;
}

inline ref_t definePrimitiveArrayItem(ref_t classRef)
{
   switch (classRef)
   {
      case V_INT32ARRAY:
      case V_INT16ARRAY:
      case V_INT8ARRAY:
         return V_INT32;
      default:
         return 0;
   }
}

inline bool isPrimitiveStructArrayRef(ref_t classRef)
{
   switch (classRef)
   {
      case V_INT32ARRAY:
      case V_INT16ARRAY:
      case V_INT8ARRAY:
      case V_BINARYARRAY:
         return true;
      default:
         return false;
   }
}

inline bool IsInvertedOperator(int& operator_id)
{
   switch (operator_id)
   {
      case NOTEQUAL_MESSAGE_ID:
         operator_id = EQUAL_MESSAGE_ID;

         return true;
      case NOTLESS_MESSAGE_ID:
         operator_id = LESS_MESSAGE_ID;

         return true;
      case NOTGREATER_MESSAGE_ID:
         operator_id = GREATER_MESSAGE_ID;

         return true;
      default:
         return false;
   }
}

inline ident_t findSourceRef(SNode node)
{
   while (node != lxNone && node != lxNamespace) {
      if (node.compare(lxConstructor, lxStaticMethod, lxClassMethod) && node.existChild(lxSourcePath)) {
         return node.findChild(lxSourcePath).identifier();
      }

      node = node.parentNode();
   }

   return node.findChild(lxSourcePath).identifier();
}

//// --- CompilerLogic Optimization Ops ---
//struct EmbeddableOp
//{
//   int attribute;
//   int paramCount;   // -1 indicates that operation should be done with the assigning target
//   int verb;
//
//   EmbeddableOp(int attr, int count, int verb)
//   {
//      this->attribute = attr;
//      this->paramCount = count;
//      this->verb = verb;
//   }
//};
//#define EMBEDDABLEOP_MAX 5
//EmbeddableOp embeddableOps[EMBEDDABLEOP_MAX] =
//{
//   EmbeddableOp(maEmbeddableGetAt, 2, READ_MESSAGE_ID),
//   EmbeddableOp(maEmbeddableGetAt2, 3, READ_MESSAGE_ID),
//   EmbeddableOp(maEmbeddableEval, 2, EVAL_MESSAGE_ID),
//   EmbeddableOp(maEmbeddableEval2, 3, EVAL_MESSAGE_ID),
//   EmbeddableOp(maEmbeddableNew, -1, 0) 
//};

// --- CompilerLogic ---

CompilerLogic :: CompilerLogic()
{
   // nil
   operators.add(OperatorInfo(EQUAL_MESSAGE_ID, V_NIL, 0, lxNilOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_MESSAGE_ID, V_NIL, 0, lxNilOp, V_FLAG));

   // int32 primitive operations
   operators.add(OperatorInfo(ADD_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(SUB_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(MUL_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(DIV_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(AND_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(OR_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(XOR_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(READ_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(WRITE_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_INT32));

   operators.add(OperatorInfo(EQUAL_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(LESS_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(NOTLESS_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(GREATER_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(NOTGREATER_MESSAGE_ID, V_INT32, V_INT32, lxIntOp, V_FLAG));

//   // subject primitive operations
//   operators.add(OperatorInfo(EQUAL_MESSAGE_ID, V_SIGNATURE, V_SIGNATURE, lxIntOp, V_FLAG));
//   operators.add(OperatorInfo(NOTEQUAL_MESSAGE_ID, V_SIGNATURE, V_SIGNATURE, lxIntOp, V_FLAG));

   // int64 primitive operations
   operators.add(OperatorInfo(ADD_MESSAGE_ID,   V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(SUB_MESSAGE_ID,   V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(MUL_MESSAGE_ID,   V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(DIV_MESSAGE_ID,   V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(AND_MESSAGE_ID,   V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(OR_MESSAGE_ID,    V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(XOR_MESSAGE_ID,   V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(READ_MESSAGE_ID,  V_INT64, V_INT32, lxLongOp, V_INT64));
   operators.add(OperatorInfo(WRITE_MESSAGE_ID, V_INT64, V_INT32, lxLongOp, V_INT64));

   operators.add(OperatorInfo(EQUAL_MESSAGE_ID, V_INT64, V_INT64, lxLongOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_MESSAGE_ID, V_INT64, V_INT64, lxLongOp, V_FLAG));
   operators.add(OperatorInfo(LESS_MESSAGE_ID, V_INT64, V_INT64, lxLongOp, V_FLAG));
   operators.add(OperatorInfo(NOTLESS_MESSAGE_ID, V_INT64, V_INT64, lxLongOp, V_FLAG));
   operators.add(OperatorInfo(GREATER_MESSAGE_ID, V_INT64, V_INT64, lxLongOp, V_FLAG));
   operators.add(OperatorInfo(NOTGREATER_MESSAGE_ID, V_INT64, V_INT64, lxLongOp, V_FLAG));

   // real64 primitive operations
   operators.add(OperatorInfo(ADD_MESSAGE_ID, V_REAL64, V_REAL64, lxRealOp, V_REAL64));
   operators.add(OperatorInfo(SUB_MESSAGE_ID, V_REAL64, V_REAL64, lxRealOp, V_REAL64));
   operators.add(OperatorInfo(MUL_MESSAGE_ID, V_REAL64, V_REAL64, lxRealOp, V_REAL64));
   operators.add(OperatorInfo(DIV_MESSAGE_ID, V_REAL64, V_REAL64, lxRealOp, V_REAL64));

   operators.add(OperatorInfo(EQUAL_MESSAGE_ID, V_REAL64, V_REAL64, lxRealOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_MESSAGE_ID, V_REAL64, V_REAL64, lxRealOp, V_FLAG));
   operators.add(OperatorInfo(LESS_MESSAGE_ID, V_REAL64, V_REAL64, lxRealOp, V_FLAG));
   operators.add(OperatorInfo(NOTLESS_MESSAGE_ID, V_REAL64, V_REAL64, lxRealOp, V_FLAG));
   operators.add(OperatorInfo(GREATER_MESSAGE_ID, V_REAL64, V_REAL64, lxRealOp, V_FLAG));
   operators.add(OperatorInfo(NOTGREATER_MESSAGE_ID, V_REAL64, V_REAL64, lxRealOp, V_FLAG));

   // array of int32 primitive operations
   operators.add(OperatorInfo(REFER_MESSAGE_ID, V_INT32ARRAY, V_INT32, lxIntArrOp, V_INT32));
   operators.add(OperatorInfo(SET_REFER_MESSAGE_ID, V_INT32ARRAY, V_INT32, V_INT32, lxIntArrOp, 0));
   operators.add(OperatorInfo(READ_MESSAGE_ID, V_INT32ARRAY, V_INT32, lxIntArrOp, 0));

   // array of int16 primitive operations
   operators.add(OperatorInfo(REFER_MESSAGE_ID, V_INT16ARRAY, V_INT32, lxShortArrOp, V_INT32));
   operators.add(OperatorInfo(SET_REFER_MESSAGE_ID, V_INT16ARRAY, V_INT32, V_INT32, lxShortArrOp, 0));
   operators.add(OperatorInfo(READ_MESSAGE_ID, V_INT16ARRAY, V_INT32, lxShortArrOp, 0));

   // array of int8 primitive operations
   operators.add(OperatorInfo(REFER_MESSAGE_ID, V_INT8ARRAY, V_INT32, lxByteArrOp, V_INT32));
   operators.add(OperatorInfo(SET_REFER_MESSAGE_ID, V_INT8ARRAY, V_INT32, V_INT32, lxByteArrOp, 0));
   operators.add(OperatorInfo(READ_MESSAGE_ID, V_INT8ARRAY, V_INT32, lxByteArrOp, 0));

   // array of object primitive operations
   operators.add(OperatorInfo(REFER_MESSAGE_ID, V_OBJARRAY, V_INT32, lxArrOp, V_OBJECT));
   operators.add(OperatorInfo(SET_REFER_MESSAGE_ID, V_OBJARRAY, V_INT32, 0, lxArrOp, 0));
   operators.add(OperatorInfo(READ_MESSAGE_ID, V_OBJARRAY, V_INT32, lxArrOp, 0));

   // array of structures primitive operations
   operators.add(OperatorInfo(REFER_MESSAGE_ID, V_BINARYARRAY, V_INT32, lxBinArrOp, V_BINARY));
   operators.add(OperatorInfo(SET_REFER_MESSAGE_ID, V_BINARYARRAY, V_INT32, 0, lxBinArrOp, 0));
   operators.add(OperatorInfo(READ_MESSAGE_ID, V_BINARYARRAY, V_INT32, lxBinArrOp, 0));

   // array of arg list
   operators.add(OperatorInfo(REFER_MESSAGE_ID, V_ARGARRAY, V_INT32, lxArgArrOp, 0));
   operators.add(OperatorInfo(SET_REFER_MESSAGE_ID, V_ARGARRAY, V_INT32, 0, lxArgArrOp, 0));

   //operators.add(OperatorInfo(READ_MESSAGE_ID, V_OBJARRAY, V_INT32, lxArrOp, 0));

   // boolean primitive operations
   operators.add(OperatorInfo(AND_MESSAGE_ID, V_FLAG, V_FLAG, 0, lxBoolOp, V_FLAG));
   operators.add(OperatorInfo(OR_MESSAGE_ID, V_FLAG, V_FLAG, 0, lxBoolOp, V_FLAG));
}

int CompilerLogic :: checkMethod(ClassInfo& info, ref_t message, ChechMethodInfo& result)
{
   bool methodFound = info.methods.exist(message);

   if (methodFound) {
      int hint = info.methodHints.get(Attribute(message, maHint));
      result.outputReference = info.methodHints.get(Attribute(message, maReference));

      result.embeddable = test(hint, tpEmbeddable);
      result.closure = test(hint, tpAction);

      if ((hint & tpMask) == tpSealed) {
         return hint;
      }
      else if (test(info.header.flags, elFinal)) {
         return tpSealed | hint;
      }
      else if (test(info.header.flags, elClosed)) {
         return tpClosed | hint;
      }
      else return tpNormal | hint;
   }
   //HOTFIX : to recognize the sealed private method call
   //         hint search should be done even if the method is not declared
   else return info.methodHints.get(Attribute(message, maHint));
}

int CompilerLogic :: checkMethod(_CompilerScope& scope, ref_t reference, ref_t message, ChechMethodInfo& result)
{
   ClassInfo info;
   result.found = defineClassInfo(scope, info, reference);

   if (result.found) {
      if (test(info.header.flags, elClosed))
         result.directResolved = true;

//      //if (!test(info.header.flags, elClosed))
//      //   result.closed = false;
//
      if (test(info.header.flags, elWithCustomDispatcher))
         result.withCustomDispatcher = true;

      int hint = checkMethod(info, message, result);
      if (hint == tpUnknown && test(info.header.flags, elWithArgGenerics)) {
         hint = checkMethod(info, overwriteParamCount(message, OPEN_ARG_COUNT), result);
         if (hint != tpUnknown) {
            result.withOpenArgDispatcher = true;
         }
         else {
            hint = checkMethod(info, overwriteParamCount(message, OPEN_ARG_COUNT + 1), result);
            if (hint != tpUnknown) {
               result.withOpenArg1Dispatcher = true;
            }
            else {
               hint = checkMethod(info, overwriteParamCount(message, OPEN_ARG_COUNT + 2), result);
               if (hint != tpUnknown) {
                  result.withOpenArg2Dispatcher = true;
               }
            }
         }
      }

      return hint;
   }
   else return tpUnknown;
}

int CompilerLogic :: resolveCallType(_CompilerScope& scope, ref_t& classReference, ref_t messageRef, ChechMethodInfo& result)
{
//   if (isPrimitiveRef(classReference)) {
//      classReference = resolvePrimitiveReference(scope, classReference);
//   }

   int methodHint = checkMethod(scope, classReference != 0 ? classReference : scope.superReference, messageRef, result);
   int callType = methodHint & tpMask;
//   if (callType == tpClosed || callType == tpSealed) {
      result.stackSafe = /*test(methodHint, tpStackSafe)*/true; // NOTE : currently all methods are stack-safe
//   }      

   if (getAction(messageRef) == INVOKE_MESSAGE_ID) {
      // HOTFIX : calling closure
      result.closure = true;
   }

   return callType;
}

int CompilerLogic :: resolveOperationType(_CompilerScope& scope, _Compiler& compiler, int operatorId, ref_t loperand, ref_t roperand, ref_t& result)
{
   if (loperand == 0 || (roperand == 0 && loperand != V_NIL))
      return 0;

   OperatorList::Iterator it = operators.start();
   while (!it.Eof()) {
      OperatorInfo info = *it;

      if (info.operatorId == operatorId) {
         if (info.loperand == V_NIL) {
            if (loperand == V_NIL) {
               result = info.result;

               return info.operationType;
            }
         }
         else if (info.loperand == V_FLAG && info.roperand == V_FLAG) {
            if (isBoolean(scope, compiler, loperand) && isBoolean(scope, compiler, loperand)) {
               result = info.result;

               return info.operationType;
            }
         }
         else if (isCompatible(scope, info.loperand, loperand) && isCompatible(scope, info.roperand, roperand)) {
            result = info.result;

            return info.operationType;
         }
      }

      it++;
   }

   return 0;
}

int CompilerLogic :: resolveOperationType(_CompilerScope& scope, int operatorId, ref_t loperand, ref_t roperand, ref_t roperand2, ref_t& result)
{
   if (loperand == 0 || roperand == 0 || (roperand2 == 0 && loperand != V_OBJARRAY))
      return 0;

   OperatorList::Iterator it = operators.start();
   while (!it.Eof()) {
      OperatorInfo info = *it;

      if (info.operatorId == operatorId) {
         if (info.loperand == V_NIL) {
            // skip operation with NIL
         }
         else if (isCompatible(scope, info.loperand, loperand) && isCompatible(scope, info.roperand, roperand)
            && isCompatible(scope, info.roperand2, roperand2)) 
         {
            result = info.result;

            return info.operationType;
         }

      }
      it++;
   }

   return 0;
}

bool CompilerLogic :: loadBranchingInfo(_CompilerScope& scope, _Compiler& compiler, ref_t reference)
{
   if (scope.branchingInfo.trueRef == reference || scope.branchingInfo.falseRef == reference)
      return true;

   ClassInfo info;
   scope.loadClassInfo(info, reference);

   if ((info.header.flags & elDebugMask) == elEnumList) {
      _Module* extModule = NULL;
      _Memory* listSection = NULL;

      while (true) {
         ref_t listRef = info.statics.get(ENUM_VAR).value1;

         ref_t extRef = 0;
         extModule = scope.loadReferenceModule(listRef, extRef);
         listSection = extModule ? extModule->mapSection(extRef | mskRDataRef, true) : NULL;

         if (listSection == NULL && info.header.parentRef != 0) {
            reference = info.header.parentRef;

            scope.loadClassInfo(info, reference);
         }
         else break;
      }

      if (listSection) {
         ref_t trueRef = 0;
         ref_t falseRef = 0;

         // read enum list member and find true / false values
         _ELENA_::RelocationMap::Iterator it(listSection->getReferences());
         ref_t currentMask = 0;
         ref_t memberRef = 0;
         while (!it.Eof()) {
            currentMask = it.key() & mskAnyRef;
            if (currentMask == mskConstantRef) {
               memberRef = importReference(extModule, it.key() & ~mskAnyRef, scope.module);

               ClassInfo memberInfo;
               scope.loadClassInfo(memberInfo, memberRef);
               int attribute = checkMethod(memberInfo, encodeMessage(IF_MESSAGE_ID, 1));
               if (attribute == (tpIfBranch | tpSealed)) {
                  trueRef = memberRef;
               }
               else if (attribute == (tpIfNotBranch | tpSealed)) {
                  falseRef = memberRef;
               }
            }

            it++;
         }

         if (trueRef && falseRef) {
            scope.branchingInfo.reference = reference;
            scope.branchingInfo.trueRef = trueRef;
            scope.branchingInfo.falseRef = falseRef;

            return true;
         }
      }
   }

   return false;
}

bool CompilerLogic :: resolveBranchOperation(_CompilerScope& scope, _Compiler& compiler, int operatorId, ref_t loperand, ref_t& reference)
{
   if (!loperand)
      return false;

   if (loperand != scope.branchingInfo.reference) {
      if (!loadBranchingInfo(scope, compiler, loperand))
         return false;
   }

   reference = operatorId == IF_MESSAGE_ID ? scope.branchingInfo.trueRef : scope.branchingInfo.falseRef;

   return true;
}

int CompilerLogic :: resolveNewOperationType(_CompilerScope& scope, ref_t loperand, ref_t roperand, ref_t& result)
{
   if (isCompatible(scope, V_INT32, roperand)) {
      result = definePrimitiveArray(scope, loperand);
      if (result != 0)
         return lxNewOp;
   }

   return 0;
}

//inline bool isPrimitiveCompatible(ref_t targetRef, ref_t sourceRef)
//{
//   switch (sourceRef)
//   {
//      case V_PTR32:
//         return targetRef == V_INT32;
//      case V_INT32:
//         return targetRef == V_PTR32;
//      default:
//         return false;
//   }
//}

//inline bool isPrimitiveArrayCompatible(ref_t targetRef, ref_t sourceRef)
//{
//   switch (sourceRef)
//   {
//      case V_PTR32:
//         return targetRef == V_INT32;
//      default:
//         return false;
//   }
//}

bool CompilerLogic :: isCompatible(_CompilerScope& scope, ref_t targetRef, ref_t sourceRef)
{
   if (!targetRef && !isPrimitiveRef(sourceRef))
      return true;

   if (sourceRef == V_NIL)
      return true;

   //if (isPrimitiveCompatible(targetRef, sourceRef))
   //   return true;

   if (targetRef == scope.superReference && !isPrimitiveRef(sourceRef))
      return true;

   while (sourceRef != 0) {
      if (targetRef != sourceRef) {
         ClassInfo info;
         if (!defineClassInfo(scope, info, sourceRef))
            return false;

         // if it is a structure wrapper
         if (isPrimitiveRef(targetRef) && test(info.header.flags, elStructureWrapper)) {
            ClassInfo::FieldInfo inner = info.fieldTypes.get(0);
            if (isCompatible(scope, targetRef, inner.value1))
               return true;
         }

         if (test(info.header.flags, elClassClass)) {
            // class class can be compatible only with itself and the super class
            sourceRef = scope.superReference;
         }
         else sourceRef = info.header.parentRef;
      }
      else return true;
   }

   return false;
}

bool CompilerLogic :: isEmbeddableArray(ClassInfo& info)
{
   return test(info.header.flags, elDynamicRole | elStructureRole);
}

bool CompilerLogic :: isVariable(_CompilerScope& scope, ref_t classReference)
{
   ClassInfo info;
   if (!defineClassInfo(scope, info, classReference))
      return false;

   return isVariable(info);
}

bool CompilerLogic :: isVariable(ClassInfo& info)
{
   return test(info.header.flags, elWrapper) && !test(info.header.flags, elReadOnlyRole);
}

bool CompilerLogic :: isEmbeddable(ClassInfo& info)
{
   return test(info.header.flags, elStructureRole) && !test(info.header.flags, elDynamicRole);
}

bool CompilerLogic :: isRole(ClassInfo& info)
{
   return test(info.header.flags, elRole);
}

bool CompilerLogic :: isAbstract(ClassInfo& info)
{
   return test(info.header.flags, elAbstract);
}

bool CompilerLogic :: isMethodStacksafe(ClassInfo& info, ref_t message)
{
   // NOTE : currently all methods are stack safe by default
   return /*test(info.methodHints.get(Attribute(message, maHint)), tpStackSafe)*/true;
}

bool CompilerLogic :: isMethodGeneric(ClassInfo& info, ref_t message)
{
   return test(info.methodHints.get(Attribute(message, maHint)), tpGeneric);
}

bool CompilerLogic :: isMultiMethod(ClassInfo& info, ref_t message)
{
   return test(info.methodHints.get(Attribute(message, maHint)), tpMultimethod);
}

bool CompilerLogic :: isClosure(ClassInfo& info, ref_t message)
{
   return test(info.methodHints.get(Attribute(message, maHint)), tpAction);
}

void CompilerLogic :: injectOverloadList(_CompilerScope& scope, ClassInfo& info, _Compiler& compiler, ref_t classRef)
{
   for (auto it = info.methods.start(); !it.Eof(); it++) {
      // if the method included
      if (*it) {
         ref_t message = it.key();
         if (getAbsoluteParamCount(message) > 0 && getAction(message) != 0 && !test(message, SPECIAL_MESSAGE)) {
            ref_t signatureRef = 0;
            ident_t actionName = scope.module->resolveAction(getAction(message), signatureRef);

            if (signatureRef) {
               ref_t actionRef = 0;
               /*if (index > 0) {
                  IdentifierString content(messageName, index);

                  */actionRef = scope.module->mapAction(actionName, 0, true);
               //}
               //else actionRef = SET_MESSAGE_ID;

               ref_t flags = message & MESSAGE_FLAG_MASK;
               ref_t listRef = info.methodHints.get(Attribute(encodeMessage(actionRef, getAbsoluteParamCount(message) | flags), maOverloadlist));
               if (listRef != 0) {
                  if (test(info.header.flags, elSealed) || test(message, SEALED_MESSAGE)) {
                     compiler.generateSealedOverloadListMember(scope, listRef, message, classRef);
                  }
                  else if (test(info.header.flags, elClosed)) {
                     compiler.generateClosedOverloadListMember(scope, listRef, message, classRef);
                  }
                  else compiler.generateOverloadListMember(scope, listRef, message);
               }
            }
         }         
      }
   }
}

void CompilerLogic :: injectVirtualFields(_CompilerScope& scope, SNode node, ref_t classRef, ClassInfo& info, _Compiler& compiler)
{
   // generate enumeration list static field
   if ((info.header.flags & elDebugMask) == elEnumList && !test(info.header.flags, elNestedClass)) {
      compiler.injectVirtualStaticConstField(scope, node, ENUM_VAR, classRef);
   }
}

void CompilerLogic :: injectVirtualCode(_CompilerScope& scope, SNode node, ref_t classRef, ClassInfo& info, _Compiler& compiler, bool closed)
{
   // generate enumeration list
   if ((info.header.flags & elDebugMask) == elEnumList && test(info.header.flags, elNestedClass)) {
      ClassInfo::FieldInfo valuesField = info.statics.get(ENUM_VAR);

      compiler.generateListMember(scope, valuesField.value1, classRef);
   }

   if (!testany(info.header.flags, elClassClass | elNestedClass) && classRef != scope.superReference && !closed && !test(info.header.flags, elExtension)) {
      // auto generate get&type message for explicitly declared classes
      ref_t signRef = scope.module->mapSignature(&classRef, 1, false);
      ref_t actionRef = scope.module->mapAction(CAST_MESSAGE, signRef, false);

      compiler.injectVirtualReturningMethod(scope, node, encodeAction(actionRef), SELF_VAR);
   }

   //// generate structure embeddable constructor
   //if (test(info.header.flags, elSealed | elStructureRole)) {
   //   bool found = false;
   //   SNode current = node.firstChild();
   //   while (current != lxNone) {
   //      if (current == lxClassMethod && test(current.argument, SPECIAL_MESSAGE) && getAbsoluteParamCount(current.argument) > 0) {
   //         SNode attr = current.firstChild();
   //         while (attr != lxNone) {
   //            if (attr == lxAttribute && attr.argument == tpEmbeddable) {
   //               current.set(lxClassMethod, encodeMessage(NEW_MESSAGE_ID, 0) | SEALED_MESSAGE);
   //               attr.argument = tpPrivate;

   //               found = true;
   //               break;
   //            }
   //            attr = attr.nextNode();
   //         }
   //         break;
   //      }
   //      current = current.nextNode();
   //   }
   //   if (found) {
   //      compiler.injectEmbeddableConstructor(node, encodeVerb(NEW_MESSAGE_ID), encodeMessage(NEW_MESSAGE_ID, 0) | SEALED_MESSAGE);
   //   }
   //}
}

void CompilerLogic :: injectVirtualMultimethods(_CompilerScope& scope, SNode node, ClassInfo& info, _Compiler& compiler, List<ref_t>& implicitMultimethods, LexicalType methodType)
{
   // generate implicit mutli methods
   for (auto it = implicitMultimethods.start(); !it.Eof(); it++) {
      if (info.methods.exist(*it)) {
         compiler.injectVirtualMultimethod(scope, node, *it, methodType, info.header.parentRef);
      }
      else compiler.injectVirtualMultimethod(scope, node, *it, methodType);

      if (isOpenArg(*it)) {
         // generate explicit argument list dispatcher
         compiler.injectVirtualArgDispatcher(scope, node, *it, methodType);

         ref_t resendMessage = encodeMessage(getAction(*it), getParamCount(*it) + 1);

         // generate argument list dispatcher multi-method
         if (info.methods.exist(resendMessage)) {
            compiler.injectVirtualMultimethod(scope, node, resendMessage, methodType, info.header.parentRef);
         }
         else compiler.injectVirtualMultimethod(scope, node, resendMessage, methodType);
      }

      info.header.flags |= elWithMuti;
   }
}

void CompilerLogic :: verifyMultimethods(_CompilerScope& scope, SNode node, ClassInfo& info, List<ref_t>& implicitMultimethods)
{
   // HOTFIX : Make sure the multi-method methods have the same output type as generic one
   bool needVerification = false;
   for (auto it = implicitMultimethods.start(); !it.Eof(); it++) {
      ref_t message = *it;

      ref_t outputRef = info.methodHints.get(Attribute(message, maReference));
      if (outputRef != 0) {
         // Bad luck we have to verify all overloaded methods
         needVerification = true;
         break;
      }
   }

   if (!needVerification)
      return;

   SNode current = node.firstChild();
   while (current != lxNone) {
      if (current == lxClassMethod) {
         SNode multiMethAttr = current.findChild(lxMultiMethodAttr);
         if (multiMethAttr != lxNone) {
            ref_t outputRefMulti = info.methodHints.get(Attribute(multiMethAttr.argument, maReference));
            if (outputRefMulti != 0) {
               ref_t outputRef = info.methodHints.get(Attribute(current.argument, maReference));
               if (outputRef == 0) {
                  scope.raiseError(errNotCompatibleMulti, findSourceRef(current), current.findChild(lxIdentifier, lxPrivate));
               }
               else if (!isCompatible(scope, outputRefMulti, outputRef)) {
                  scope.raiseError(errNotCompatibleMulti, findSourceRef(current), current.findChild(lxIdentifier, lxPrivate));
               }
            }            
         }
      }
      current = current.nextNode();
   }
}

bool CompilerLogic :: isBoolean(_CompilerScope& scope, _Compiler& compiler, ref_t reference)
{
   if (!scope.branchingInfo.reference) {
      // HOTFIX : resolve boolean symbols
      ref_t dummy;
      resolveBranchOperation(scope, compiler, IF_MESSAGE_ID, scope.boolReference, dummy);
   }

   return isCompatible(scope, scope.branchingInfo.reference, reference);
}

void CompilerLogic :: injectOperation(SyntaxWriter& writer, _CompilerScope& scope, _Compiler& compiler, int operator_id, int operationType, ref_t& reference, ref_t elementRef)
{
   int size = 0;
   if (operationType == lxBinArrOp) {
      // HOTFIX : define an item size for the binary array operations
      size = -defineStructSize(scope, V_BINARYARRAY, elementRef);
   }

   if (reference == V_BINARY && elementRef != 0) {
      reference = elementRef;
   }
   else if (reference == V_OBJECT && elementRef != 0) {
      reference = elementRef;
   }

   bool inverting = IsInvertedOperator(operator_id);

   if (reference == V_FLAG) {      
      if (!scope.branchingInfo.reference) {
         // HOTFIX : resolve boolean symbols
         ref_t dummy;
         resolveBranchOperation(scope, compiler, IF_MESSAGE_ID, scope.boolReference, dummy);
      }

      reference = scope.branchingInfo.reference;
      if (inverting) {
         writer.appendNode(lxIfValue, scope.branchingInfo.falseRef);
         writer.appendNode(lxElseValue, scope.branchingInfo.trueRef);
      }
      else {
         writer.appendNode(lxIfValue, scope.branchingInfo.trueRef);
         writer.appendNode(lxElseValue, scope.branchingInfo.falseRef);
      }
   }

   if (size != 0) {
      // HOTFIX : inject an item size for the binary array operations
      writer.appendNode(lxSize, size);
   }

   writer.insert((LexicalType)operationType, operator_id);
   writer.closeNode();
}

bool CompilerLogic :: isReadonly(ClassInfo& info)
{
   return test(info.header.flags, elReadOnlyRole);
}

bool CompilerLogic :: injectImplicitCreation(SyntaxWriter& writer, _CompilerScope& scope, _Compiler& compiler, ref_t targetRef)
{
   ClassInfo info;
   if (!defineClassInfo(scope, info, targetRef))
      return false;

   if (test(info.header.flags, elStateless))
      return false;

   ref_t implicitConstructor = encodeMessage(DEFAULT_MESSAGE_ID, 0) | SPECIAL_MESSAGE;
   if (!info.methods.exist(implicitConstructor, true))
      return false;

   bool stackSafe = isMethodStacksafe(info, implicitConstructor);

   if (test(info.header.flags, elStructureRole)) {
      //compiler.injectConverting(writer, lxDirectCalling, implicitConstructor, lxCreatingStruct, info.size, targetRef, stackSafe);
      compiler.injectConverting(writer, lxNone, 0, lxImplicitCall, encodeAction(NEWOBJECT_MESSAGE_ID), info.header.classRef, targetRef, stackSafe);

      return true;
   }
   else if (test(info.header.flags, elDynamicRole)) {
      return false;
   }
   else {
      //compiler.injectConverting(writer, lxDirectCalling, implicitConstructor, lxCreatingClass, info.fields.Count(), targetRef*//*, stackSafe*/);
      compiler.injectConverting(writer, lxNone, 0, lxImplicitCall, encodeAction(NEWOBJECT_MESSAGE_ID), info.header.classRef, targetRef, stackSafe);

      return true;
   }
}

bool CompilerLogic :: isSignatureCompatible(_CompilerScope& scope, ref_t targetSignature, ref_t* sourceSignatures)
{
   ref_t targetSignatures[OPEN_ARG_COUNT];
   size_t len = scope.module->resolveSignature(targetSignature, targetSignatures);
   if (len < 0)
      return false;

   for (size_t i = 0; i < len; i++) {
      if (!isCompatible(scope, targetSignatures[i], sourceSignatures[i]))
         return false;
   }

   return true;
}

bool CompilerLogic :: isSignatureCompatible(_CompilerScope& scope, _Module* targetModule, ref_t targetSignature, ref_t* sourceSignatures)
{
   ref_t targetSignatures[OPEN_ARG_COUNT];
   size_t len = targetModule->resolveSignature(targetSignature, targetSignatures);

   for (size_t i = 0; i < len; i++) {
      if (!isCompatible(scope, importReference(targetModule, targetSignatures[i], scope.module), sourceSignatures[i]))
         return false;
   }

   return true;

}

bool CompilerLogic :: injectImplicitConstructor(SyntaxWriter& writer, _CompilerScope& scope, _Compiler& compiler, ClassInfo& info, ref_t targetRef, ref_t elementRef, ref_t* signatures, int paramCount)
{
   ClassInfo::MethodMap::Iterator it = info.methods.start();
   while (!it.Eof()) {
      pos_t implicitMessage = it.key();
      if (test(implicitMessage, SPECIAL_MESSAGE) && getParamCount(implicitMessage) == paramCount) {
//         ref_t subj = getAction(implicitMessage);
         ref_t signatureRef = 0;
         scope.module->resolveAction(getAction(implicitMessage), signatureRef);
         bool compatible = false;
         //if (actionRef == V_STRCONSTANT && subj == elementRef) {
         //   // try to resolve explicit constant conversion routine
         //   compatible = true;
         //}
         //else if (paramCount == 1) {
         //   ref_t subjRef = scope.module->mapReference(scope.module->resolveSubject(subj).c_str() + 1);
         //   if (subjRef != 0) {
         //      compatible = isCompatible(scope, subjRef, actionRef);
         //   }
         //   else compatible = true;
         //}
         /*else */compatible = isSignatureCompatible(scope, signatureRef, signatures);

         if (compatible) {
            bool stackSafe = isMethodStacksafe(info, implicitMessage);
            if (test(info.header.flags, elStructureRole)) {
            //   compiler.injectConverting(writer, lxDirectCalling, implicitMessage, lxCreatingStruct, info.size, targetRef, stackSafe);
               compiler.injectConverting(writer, lxDirectCalling, implicitMessage, lxImplicitCall, encodeAction(NEWOBJECT_MESSAGE_ID), 
                  info.header.classRef, targetRef, stackSafe);
            }
            else if (test(info.header.flags, elDynamicRole)) {
               return false;
            }
            else compiler.injectConverting(writer, lxDirectCalling, implicitMessage, lxImplicitCall, encodeAction(NEWOBJECT_MESSAGE_ID), 
               info.header.classRef, targetRef, stackSafe);

            return true;
         }
      }

      it++;
   }

   return false;
}

//bool CompilerLogic :: injectImplicitConstructor(SyntaxWriter& writer, _CompilerScope& scope, _Compiler& compiler, ref_t targetRef, ref_t actionRef, int paramCount)
//{
//   ClassInfo info;
//   if (!defineClassInfo(scope, info, targetRef))
//      return false;
//
//   return injectImplicitConstructor(writer, scope, compiler, info, targetRef, 0, actionRef, paramCount);
//}

bool CompilerLogic :: injectImplicitConversion(SyntaxWriter& writer, _CompilerScope& scope, _Compiler& compiler, ref_t targetRef, ref_t sourceRef, ref_t elementRef)
{
   if (targetRef == 0 && isPrimitiveRef(sourceRef)) {
      if (isPrimitiveArrayRef(sourceRef)) {
         // HOTFIX : replace generic object with a generic array
         targetRef = scope.arrayReference;
      }
      else if (sourceRef == V_INT32) {
         // HOTFIX : replace generic object with an integer constant
         targetRef = scope.intReference;
      }
      else if (sourceRef == V_INT64) {
         // HOTFIX : replace generic object with an integer constant
         targetRef = scope.longReference;
      }
      else if (sourceRef == V_REAL64) {
         // HOTFIX : replace generic object with an integer constant
         targetRef = scope.realReference;
      }
      else if (sourceRef == V_MESSAGE) {
         targetRef = scope.messageReference;
      }
//      else if (sourceRef == V_SIGNATURE) {
//         targetRef = scope.signatureReference;
//      }
   }

   ClassInfo info;
   if (!defineClassInfo(scope, info, targetRef))
      return false;

   // if the target class is wrapper around the source
   if (test(info.header.flags, elWrapper)) {
      ClassInfo::FieldInfo inner = info.fieldTypes.get(0);

      bool compatible = false;
      if (test(info.header.flags, elStructureWrapper)) {
         if (isPrimitiveRef(sourceRef)) {
            compatible = isCompatible(scope, sourceRef, inner.value1);
         }
         // HOTFIX : the size should be taken into account as well (e.g. byte and int both V_INT32)
         else compatible = isCompatible(scope, inner.value1, sourceRef) && info.size == defineStructSize(scope, sourceRef, 0u);
      }
      else compatible = isCompatible(scope, inner.value1, sourceRef);

      if (compatible) {
         compiler.injectBoxing(writer, scope, 
            isReadonly(info) ? lxBoxing : lxUnboxing,
            test(info.header.flags, elStructureRole) ? info.size : 0, targetRef);

         return true;
      }
   }

   // HOTFIX : trying to typecast primitive structure array
   if (isPrimitiveStructArrayRef(sourceRef) && test(info.header.flags, elStructureRole | elDynamicRole)) {
      ClassInfo sourceInfo;      
      if (sourceRef == V_BINARYARRAY && elementRef != 0) {
         if (defineClassInfo(scope, sourceInfo, elementRef, true)) {
            if (-sourceInfo.size == info.size && isCompatible(scope, elementRef, info.fieldTypes.get(-1).value1)) {
               compiler.injectBoxing(writer, scope,
                  test(info.header.flags, elReadOnlyRole) ? lxBoxing : lxUnboxing, info.size, targetRef);

               return true;
            }
         }
      }
      else {
         if (defineClassInfo(scope, sourceInfo, sourceRef, true)) {
            if (sourceInfo.size == info.size && isCompatible(scope, definePrimitiveArrayItem(sourceRef), info.fieldTypes.get(-1).value1)) {
               compiler.injectBoxing(writer, scope,
                  test(info.header.flags, elReadOnlyRole) ? lxBoxing : lxUnboxing, info.size, targetRef);

               return true;
            }
         }
      }
   }

   // HOTFIX : trying to typecast primitive array
   if (isPrimitiveArrayRef(sourceRef) && test(info.header.flags, elDynamicRole | elNonStructureRole)) {
      //ClassInfo sourceInfo;
      //defineClassInfo(scope, sourceInfo, sourceRef, true);

      if (isCompatible(scope, info.fieldTypes.get(-1).value1, elementRef)) {
         compiler.injectBoxing(writer, scope,
            test(info.header.flags, elReadOnlyRole) ? lxBoxing : lxUnboxing, 0, targetRef, true);

         return true;
      }
   }
   // HOTFIX : trying to typecast open argument list
   if (isOpenArgRef(sourceRef) && test(info.header.flags, elDynamicRole | elNonStructureRole)) {
      if (isCompatible(scope, info.fieldTypes.get(-1).value1, elementRef)) {
         compiler.injectBoxing(writer, scope, lxArgBoxing, 0, targetRef, true);

         return true;
      }
   }

   // check if there are implicit constructors
   if (sourceRef == V_OBJARRAY) {
      // HOTFIX : recognize primitive object array
      sourceRef = firstNonZero(scope.arrayReference, scope.superReference);

      compiler.injectBoxing(writer, scope,
         test(info.header.flags, elReadOnlyRole) ? lxBoxing : lxUnboxing, 0, sourceRef, true);
   }
   // HOTFIX : recognize primitive data except of a constant literal
   else if (isPrimitiveRef(sourceRef)/* && sourceRef != V_STRCONSTANT*/)
      sourceRef = resolvePrimitiveReference(scope, sourceRef);

   // otherwise we have to go through the list

   return injectImplicitConstructor(writer, scope, compiler, info, targetRef, elementRef, &sourceRef, 1);
}

void CompilerLogic :: injectNewOperation(SyntaxWriter& writer, _CompilerScope& scope, int operation, ref_t targetRef, ref_t elementRef)
{
   int size = defineStructSize(scope, targetRef, elementRef);
   if (size != 0)
      writer.appendNode(lxSize, size);

   writer.insert((LexicalType)operation, targetRef);
   writer.closeNode();
}

bool CompilerLogic :: defineClassInfo(_CompilerScope& scope, ClassInfo& info, ref_t reference, bool headerOnly)
{
   if (isPrimitiveRef(reference) && !headerOnly) {
      scope.loadClassInfo(info, scope.superReference);
   }

   switch (reference)
   {
      case V_INT32:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugDWORD | elStructureRole;
         info.size = 4;
         break;
      case V_INT64:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugQWORD | elStructureRole;
         info.size = 8;
         break;
      case V_REAL64:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugReal64 | elStructureRole;
         info.size = 8;
         break;
      case V_PTR:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugPTR | elStructureRole;
         info.size = 4;
         break;
//      case V_SIGNATURE:
//         info.header.parentRef = scope.superReference;
//         info.header.flags = elDebugSubject | elStructureRole;
//         info.size = 4;
//         break;
      case V_MESSAGE:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugMessage | elStructureRole;
         info.size = 4;
         break;
//      case V_EXTMESSAGE:
//         info.header.parentRef = scope.superReference;
//         info.header.flags = elDebugMessage | elStructureRole;
//         info.size = 8;
//         break;
//      case V_SYMBOL:
//         info.header.parentRef = scope.superReference;
//         info.header.flags = elDebugReference | elStructureRole;
//         info.size = 4;
//         break;
      case V_INT32ARRAY:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugIntegers | elStructureRole | elDynamicRole;
         info.size = -4;
         break;
      case V_INT16ARRAY:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugShorts | elStructureRole | elDynamicRole;
         info.size = -2;
         break;
      case V_INT8ARRAY:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugBytes | elStructureRole | elDynamicRole;
         info.size = -1;
         break;
      case V_OBJARRAY:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugArray | elDynamicRole;
         info.size = 0;
         break;
      case V_BINARYARRAY:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDynamicRole | elStructureRole;
         info.size = -1;
         break;
      case V_AUTO:
         break;
      default:
         if (reference != 0) {
            if (!scope.loadClassInfo(info, reference, headerOnly))
               return false;
         }
         else {
            info.header.parentRef = 0;
            info.header.flags = 0;
            info.size = 0;
         }
         break;
   }

   return true;
}

int CompilerLogic :: defineStructSizeVariable(_CompilerScope& scope, ref_t reference, ref_t elementRef, bool& variable)
{
   if (reference == V_BINARYARRAY && elementRef != 0) {
      return -defineStructSizeVariable(scope, elementRef, 0, variable);
   }
   //else if (reference == V_OBJARRAY && elementRef != 0) {
   //   return defineStructSizeVariable(scope, elementRef, 0, variable);
   //}
   else if (reference == V_INT32ARRAY) {
      return -4;
   }
   else if (reference == V_INT16ARRAY) {
      return -2;
   }
   else if (reference == V_INT8ARRAY) {
      return -1;
   }
   else {
      ClassInfo classInfo;
      if (defineClassInfo(scope, classInfo, reference)) {
         return defineStructSize(classInfo, variable);
      }
      else return 0;      
   }
}

int CompilerLogic :: defineStructSize(ClassInfo& info, bool& variable)
{
   variable = !test(info.header.flags, elReadOnlyRole);
   
   if (isEmbeddable(info)) {
      return info.size;
   }

   return 0;
}

void CompilerLogic :: tweakClassFlags(_CompilerScope& scope, _Compiler& compiler, ref_t classRef, ClassInfo& info, bool classClassMode)
{
   if (classClassMode) {
      // class class is always stateless and final
      info.header.flags |= elStateless;
      info.header.flags |= elFinal;
   }

   if (test(info.header.flags, elNestedClass)) {
      // stateless inline class
      if (info.fields.Count() == 0/* && !test(info.header.flags, elStructureRole)*/) {
         info.header.flags |= elStateless;

         // stateless inline class is its own class class
         info.header.classRef = classRef;
      }
      else info.header.flags &= ~elStateless;

      // nested class is sealed
      info.header.flags |= elSealed;
   }

   if (test(info.header.flags, elExtension)) {
      info.header.flags |= elSealed;
   }

   // verify if the class may be a wrapper
   if (isWrappable(info.header.flags) && info.fields.Count() == 1 &&
      test(info.methodHints.get(Attribute(encodeAction(DISPATCH_MESSAGE_ID), maHint)), tpEmbeddable))
   {
      if (test(info.header.flags, elStructureRole)) {
         ClassInfo::FieldInfo field = *info.fieldTypes.start();

         ClassInfo fieldInfo;
         if (defineClassInfo(scope, fieldInfo, field.value1, true) && isEmbeddable(fieldInfo)) {
            // wrapper around embeddable object should be marked as embeddable wrapper
            info.header.flags |= elEmbeddableWrapper;

            if ((info.header.flags & elDebugMask) == 0)
               info.header.flags |= fieldInfo.header.flags & elDebugMask;
         }
      }
      else info.header.flags |= elWrapper;
   }

   // adjust literal wrapper
   if ((info.header.flags & elDebugMask) == elDebugLiteral) {
      info.header.flags &= ~elDebugMask;
      if (info.size == -2) {
         info.header.flags |= elDebugWideLiteral;
      }
      else if (info.size == -1) {
         info.header.flags |= elDebugLiteral;
      }
   }

   // adjust array
   if (test(info.header.flags, elDynamicRole) && !testany(info.header.flags, elStructureRole/* | elNonStructureRole*/)) {
      info.header.flags |= elNonStructureRole;

      if ((info.header.flags & elDebugMask) == 0) {
         info.header.flags |= elDebugArray;
      }
   }

   // adjust binary array
   if (test(info.header.flags, elDynamicRole | elStructureRole)) {
      if ((info.header.flags & elDebugMask) == 0) {
         ref_t itemRef = info.fieldTypes.get(-1).value1;
         if (isCompatible(scope, V_INT32, itemRef)) {
            switch (info.size) {
               case -4:
                  info.header.flags |= elDebugIntegers;
                  break;
               case -2:
                  info.header.flags |= elDebugShorts;
                  break;
               case -1:
               default:
                  info.header.flags |= elDebugBytes;
                  break;
            }
         }
         else info.header.flags |= elDebugBytes;
      }
   }

   // adjust objects with custom dispatch handler
   if (info.methods.exist(encodeAction(DISPATCH_MESSAGE_ID), true) && classRef != scope.superReference) {
      info.header.flags |= elWithCustomDispatcher;
   }

   // generation operation list if required
   if (test(info.header.flags, elWithMuti)) {
      injectOverloadList(scope, info, compiler, classRef);
   }
}

bool CompilerLogic :: validateClassAttribute(int& attrValue)
{
   switch ((size_t)attrValue)
   {
      case V_SEALED:
         attrValue = elSealed;
         return true;
      case V_ABSTRACT:
         attrValue = elAbstract;
         return true;
//      case V_LIMITED:
//         attrValue = elClosed;
//         return true;
      case V_STRUCT:
         attrValue = elStructureRole;
         return true;
      case V_ENUMLIST:
         attrValue = /*elAbstract | */elEnumList | elClosed;
         return true;
//      case V_DYNAMIC:
//         attrValue = elDynamicRole;
//         return true;
      case V_STRING:
         attrValue = elDebugLiteral;
         return true;
      case V_CONST:
         attrValue = elReadOnlyRole;
         return true;
      case V_EXTENSION:
         attrValue = elExtension;
         return true;
//      case V_NOSTRUCT:
//         attrValue = elNonStructureRole;
//         return true;
//      case V_GROUP:
//         attrValue = elGroup;
//         return true;
//      case V_TAPEGROUP:
//         attrValue = elTapeGroup;
//         return true;
      case V_CLASS:
      case V_PUBLIC:
      case V_PRIVATE:
         attrValue = 0;
         return true;
      case V_SINGLETON:
         attrValue = elRole | elNestedClass;
         return true;
      default:
         return false;
   }
}

bool CompilerLogic :: validateMethodAttribute(int& attrValue)
{
   switch ((size_t)attrValue)
   {
      case V_IFBRANCH:
         attrValue = tpIfBranch;
         return true;
      case V_IFNOTBRANCH:
         attrValue = tpIfNotBranch;
         return true;
      //case V_STATCKSAFE:
      //   attrValue = tpStackSafe;
      //   return true;
      case V_EMBEDDABLE:
         attrValue = tpEmbeddable;
         return true;
      //case V_GENERIC:
      //   attrValue = (tpGeneric | tpSealed);
      //   return true;
      case V_PRIVATE:
         attrValue = (tpPrivate | tpSealed);
         return true;
      case V_SEALED:
         attrValue = tpSealed;
         return true;
      //case V_ACTION:
      //   attrValue = tpAction;
      //   return true;
      case V_CONSTRUCTOR:
         attrValue = tpConstructor;
         return true;
      case V_CONVERSION:
         attrValue = (tpConversion | tpSealed);
         return true;
      case V_IMPLICIT:
         attrValue = (tpSpecial | tpSealed);
         return true;
         //case V_MULTI:
      //   // obsolete
      //   attrValue = /*tpMultimethod*/0;
      //   return true;
      case V_METHOD:
         attrValue = 0;
         return true;
      case V_STATIC:
         attrValue = (tpStatic | tpSealed);
         return true;
      case V_SET:
         attrValue = tpAccessor;
         return true;
      default:
         return false;
   }
}

bool CompilerLogic :: validateFieldAttribute(int& attrValue, bool& isSealed, bool& isConstant)
{
   switch ((size_t)attrValue)
   {
      case V_STATIC:
         attrValue = lxStaticAttr;
         return true;
      case V_SEALED:
         if (!isSealed) {
            attrValue = -1;
            isSealed = true;
            return true;
         }
         else return false;
      case V_CONST:
         if (!isConstant) {
            attrValue = -1;
            isConstant = true;
            return true;
         }
         else return false;
      case V_INT8:
         attrValue = 0;
         return true;
      //case V_INT64:
      //   attrValue = 0;
      //   return true;
      case V_REAL64:
         attrValue = 0;
         return true;
      case V_PTR:
         attrValue = 0;
         return true;
      //case V_SIGNATURE:
      //   attrValue = 0;
      //   return true;
      //case V_SYMBOL:
      //   attrValue = 0;
      //   return true;
      case V_MESSAGE:
         attrValue = 0;
         return true;
      //case V_EXTMESSAGE:
      //   attrValue = 0;
      //   return true;
      //case V_OBJARRAY:
      //   return true;
      case V_FIELD:
         attrValue = -1;
         return true;
      default:
         return false;
   }
}

bool CompilerLogic :: validateLocalAttribute(int& attrValue)
{
   if (attrValue == (int)V_INT8) {
      return true;
   }
   //else if (attrValue == (int)V_OBJARRAY) {
   //   return true;
   //}
   else if (attrValue == (int)V_VARIABLE) {
      attrValue = 0;

      return true;
   }
   else if (attrValue == (int)V_AUTO) {
      return true;
   }
   else return false;
}

//bool CompilerLogic :: validateSymbolAttribute(int attrValue, bool& constant, bool& staticOne, bool& preloadedOne)
//{
//   if (attrValue == (int)V_CONST) {
//      constant = true;
//
//      return true;
//   }
//   else if (attrValue == (int)V_SYMBOLEXPR) {
//      return true;
//   }
//   else if (attrValue == (int)V_STATIC) {
//      staticOne = true;
//
//      return true;
//   }
//   else if (attrValue == (int)V_PRELOADED) {
//      preloadedOne = true;
//
//      return true;
//   }
//   else return false;
//}

////bool CompilerLogic :: validateWarningAttribute(int& attrValue)
////{
////   switch ((size_t)attrValue)
////   {
////      case V_WARNING1:
////         attrValue = WARNING_MASK_0;
////         return true;
////      case V_WARNING2:
////         attrValue = WARNING_MASK_1;
////         return true;
////      case V_WARNING3:
////         attrValue = WARNING_MASK_2;
////         return true;
////      default:
////         return false;
////   }
////}

bool CompilerLogic :: tweakPrimitiveClassFlags(ref_t classRef, ClassInfo& info)
{
   // if it is a primitive field
   if (info.fields.Count() == 1) {
      switch (classRef) {
         case V_INT32:
            info.header.flags |= (elDebugDWORD | elReadOnlyRole | elWrapper);
            info.fieldTypes.add(0, ClassInfo::FieldInfo(V_INT32, 0));
            return true;
         case V_INT64:
            info.header.flags |= (elDebugQWORD | elReadOnlyRole | elWrapper);
            info.fieldTypes.add(0, ClassInfo::FieldInfo(V_INT64, 0));
            return true;
         case V_REAL64:
            info.header.flags |= (elDebugReal64 | elReadOnlyRole | elWrapper);
            info.fieldTypes.add(0, ClassInfo::FieldInfo(V_REAL64, 0));
            return true;
         case V_PTR:
            info.header.flags |= (elDebugPTR | elWrapper);
            info.fieldTypes.add(0, ClassInfo::FieldInfo(V_PTR, 0));
            return info.size == 4;
         //case V_SIGNATURE:
         //   info.header.flags |= (elDebugSubject | elReadOnlyRole | elWrapper | elSignature);
         //   info.fieldTypes.add(0, ClassInfo::FieldInfo(V_SIGNATURE, 0));
         //   return info.size == 4;
         case V_MESSAGE:
            info.header.flags |= (elDebugMessage | elReadOnlyRole | elWrapper | elMessage);
            info.fieldTypes.add(0, ClassInfo::FieldInfo(V_MESSAGE, 0));
            return info.size == 4;
         //case V_EXTMESSAGE:
         //   info.header.flags |= (elDebugMessage | elReadOnlyRole | elWrapper | elExtMessage);
         //   info.fieldTypes.add(0, ClassInfo::FieldInfo(V_EXTMESSAGE, 0));
         //   return info.size == 8;
         //case V_SYMBOL:
         //   info.header.flags |= (elDebugReference | elReadOnlyRole | elWrapper | elSymbol);
         //   info.fieldTypes.add(0, ClassInfo::FieldInfo(V_SYMBOL, 0));
         //   return info.size == 4;
         default:
            break;
      }
   }

   return false;
}

ref_t CompilerLogic :: resolvePrimitiveReference(_CompilerScope& scope, ref_t reference)
{
   switch (reference) {
      case V_INT32:
         return firstNonZero(scope.intReference, scope.superReference);
      case V_INT64:
         return firstNonZero(scope.longReference, scope.superReference);
      case V_REAL64:
         return firstNonZero(scope.realReference, scope.superReference);
      //case V_SIGNATURE:
      //   return firstNonZero(scope.signatureReference, scope.superReference);
      case V_MESSAGE:
         return firstNonZero(scope.messageReference, scope.superReference);
      case V_ARGARRAY:
         return firstNonZero(scope.arrayReference, scope.superReference);
      default:
         return scope.superReference;
   }
}

ref_t CompilerLogic :: retrievePrimitiveReference(_CompilerScope&, ClassInfo& info)
{
   if (test(info.header.flags, elStructureWrapper)) {
      ClassInfo::FieldInfo field = info.fieldTypes.get(0);
      if (isPrimitiveRef(field.value1))
         return field.value1;
   }

   return 0;
}

ref_t CompilerLogic :: definePrimitiveArray(_CompilerScope& scope, ref_t elementRef)
{
   ClassInfo info;
   if (!defineClassInfo(scope, info, elementRef, true))
      return 0;

   if (isEmbeddable(info)) {
      if (isCompatible(scope, V_INT32, elementRef)) {
         switch (info.size) {
            case 4:
               return V_INT32ARRAY;
            case 2:
               return V_INT16ARRAY;
            case 1:
               return V_INT8ARRAY;
            default:
               break;
         }
      }
      return V_BINARYARRAY;
   }
   else return V_OBJARRAY;
}

////bool CompilerLogic :: validateClassFlag(ClassInfo& info, int flag)
////{
////   if (test(flag, elDynamicRole) && info.fields.Count() != 0)
////      return false;
////
////   return true;
////}
//
//bool CompilerLogic :: recognizeEmbeddableGet(_CompilerScope& scope, SNode root, ref_t extensionRef, ref_t returningRef, ref_t& subject)
//{
//   if (returningRef != 0 && defineStructSize(scope, returningRef, 0) > 0) {
//      root = root.findChild(lxNewFrame);
//
//      if (root.existChild(lxReturning)) {
//         SNode message = SyntaxTree::findPattern(root, 2,
//            SNodePattern(lxExpression),
//            SNodePattern(lxDirectCalling, lxSDirctCalling));
//
//         // if it is eval&subject2:var[1] message
//         if (getParamCount(message.argument) != 1)
//            return false;
//
//         // check if it is operation with $self
//         SNode target = SyntaxTree::findPattern(root, 3,
//            SNodePattern(lxExpression),
//            SNodePattern(lxDirectCalling, lxSDirctCalling),
//            SNodePattern(lxThisLocal, lxLocal));
//         if (target == lxNone) {
//            target = SyntaxTree::findPattern(root, 4,
//               SNodePattern(lxExpression),
//               SNodePattern(lxDirectCalling, lxSDirctCalling),
//               SNodePattern(lxExpression),
//               SNodePattern(lxThisLocal, lxLocal));
//         }
//
//         if (target == lxLocal && target.argument == -1 && extensionRef != 0) {            
//            if (message.findChild(lxCallTarget).argument != extensionRef)
//               return false;
//         }
//         else if (target != lxThisLocal || target.argument != 1)
//            return false;
//
//         // check if the argument is returned
//         SNode arg = SyntaxTree::findPattern(root, 4,
//            SNodePattern(lxExpression),
//            SNodePattern(lxDirectCalling, lxSDirctCalling),
//            SNodePattern(lxExpression),
//            SNodePattern(lxLocalAddress));
//
//         if (arg == lxNone) {
//            arg = SyntaxTree::findPattern(root, 5,
//               SNodePattern(lxExpression),
//               SNodePattern(lxDirectCalling, lxSDirctCalling),
//               SNodePattern(lxExpression),
//               SNodePattern(lxExpression),
//               SNodePattern(lxLocalAddress));
//         }
//
//         SNode ret = SyntaxTree::findPattern(root, 3,
//            SNodePattern(lxReturning),
//            SNodePattern(lxBoxing),
//            SNodePattern(lxLocalAddress));
//
//         if (arg != lxNone && ret != lxNone && arg.argument == ret.argument) {
//            subject = getAction(message.argument);
//
//            return true;
//         }
//      }
//   }
//
//   return false;
//}
//
//bool CompilerLogic :: recognizeEmbeddableOp(_CompilerScope& scope, SNode root, ref_t extensionRef, ref_t returningRef, ref_t verb, ref_t& subject)
//{
//   if (returningRef != 0 && defineStructSize(scope, returningRef, 0) > 0) {
//      root = root.findChild(lxNewFrame);
//
//      if (root.existChild(lxReturning))
//      {
//         SNode message = SyntaxTree::findPattern(root, 2,
//            SNodePattern(lxExpression),
//            SNodePattern(lxDirectCalling, lxSDirctCalling));
//
//         // if it is read&subject&var[2] message
//         if (getParamCount(message.argument) != 2 || getAction(message.argument) != (ref_t)verb)
//            return false;
//
//         // check if it is operation with $self
//         SNode target = SyntaxTree::findPattern(root, 3,
//            SNodePattern(lxExpression),
//            SNodePattern(lxDirectCalling, lxSDirctCalling),
//            SNodePattern(lxThisLocal, lxLocal));
//
//         if (target == lxLocal && target.argument == -1 && extensionRef != 0) {
//            if (message.findChild(lxCallTarget).argument != extensionRef)
//               return false;
//         }
//         else if (target != lxThisLocal || target.argument != 1)
//            return false;
//
//         // check if the index is used
//         SNode indexArg = target.nextNode(lxObjectMask);
//
//         if (indexArg == lxExpression)
//            indexArg = indexArg.firstChild(lxObjectMask);
//
//         if (indexArg.type != lxLocal || indexArg.argument != (ref_t)-2)
//            return false;
//
//         // check if the argument is returned
//         SNode arg = SyntaxTree::findPattern(root, 4,
//            SNodePattern(lxExpression),
//            SNodePattern(lxDirectCalling, lxSDirctCalling),
//            SNodePattern(lxExpression),
//            SNodePattern(lxLocalAddress));
//
//         if (arg == lxNone) {
//            arg = SyntaxTree::findPattern(root, 5,
//               SNodePattern(lxExpression),
//               SNodePattern(lxDirectCalling, lxSDirctCalling),
//               SNodePattern(lxExpression),
//               SNodePattern(lxExpression),
//               SNodePattern(lxLocalAddress));
//         }
//
//         SNode ret = SyntaxTree::findPattern(root, 3,
//            SNodePattern(lxReturning),
//            SNodePattern(lxBoxing),
//            SNodePattern(lxLocalAddress));
//
//         if (arg != lxNone && ret != lxNone && arg.argument == ret.argument) {
//            subject = getAction(message.argument);
//
//            return true;
//         }
//      }
//   }
//
//   return false;
//}
//
//bool CompilerLogic :: recognizeEmbeddableOp2(_CompilerScope& scope, SNode root, ref_t extensionRef, ref_t returningRef, ref_t verb, ref_t& subject)
//{
//   if (returningRef != 0 && defineStructSize(scope, returningRef, 0) > 0) {
//      root = root.findChild(lxNewFrame);
//
//      if (root.existChild(lxReturning))
//      {
//         SNode message = SyntaxTree::findPattern(root, 2,
//            SNodePattern(lxExpression),
//            SNodePattern(lxDirectCalling, lxSDirctCalling));
//
//         // if it is read&index1&index2&var[2] message
//         if (getParamCount(message.argument) != 3 || getAction(message.argument) != verb)
//            return false;
//
//         // check if it is operation with $self
//         SNode target = SyntaxTree::findPattern(root, 3,
//            SNodePattern(lxExpression),
//            SNodePattern(lxDirectCalling, lxSDirctCalling),
//            SNodePattern(lxThisLocal, lxLocal));
//
//         if (target == lxLocal && target.argument == -1 && extensionRef != 0) {
//            if (message.findChild(lxCallTarget).argument != extensionRef)
//               return false;
//         }
//         else if (target != lxThisLocal || target.argument != 1)
//            return false;
//
//         // check if the index is used
//         SNode index1Arg = target.nextNode(lxObjectMask);
//         SNode index2Arg = index1Arg.nextNode(lxObjectMask);
//
//         if (index1Arg == lxExpression)
//            index1Arg = index1Arg.firstChild(lxObjectMask);
//
//         if (index2Arg == lxExpression)
//            index2Arg = index2Arg.firstChild(lxObjectMask);
//
//         if (index1Arg.type != lxLocal || index1Arg.argument != (ref_t)-2)
//            return false;
//
//         if (index2Arg.type != lxLocal || index2Arg.argument != (ref_t)-3)
//            return false;
//
//         // check if the argument is returned
//         SNode arg = SyntaxTree::findPattern(root, 4,
//            SNodePattern(lxExpression),
//            SNodePattern(lxDirectCalling, lxSDirctCalling),
//            SNodePattern(lxExpression),
//            SNodePattern(lxLocalAddress));
//
//         if (arg == lxNone) {
//            arg = SyntaxTree::findPattern(root, 5,
//               SNodePattern(lxExpression),
//               SNodePattern(lxDirectCalling, lxSDirctCalling),
//               SNodePattern(lxExpression),
//               SNodePattern(lxExpression),
//               SNodePattern(lxLocalAddress));
//         }
//
//         SNode ret = SyntaxTree::findPattern(root, 3,
//            SNodePattern(lxReturning),
//            SNodePattern(lxBoxing),
//            SNodePattern(lxLocalAddress));
//
//         if (arg != lxNone && ret != lxNone && arg.argument == ret.argument) {
//            subject = getAction(message.argument);
//
//            return true;
//         }
//      }
//   }
//
//   return false;
//}
//
//bool CompilerLogic :: recognizeEmbeddableGetAt(_CompilerScope& scope, SNode root, ref_t extensionRef, ref_t returningRef, ref_t& subject)
//{
//   return recognizeEmbeddableOp(scope, root, extensionRef, returningRef, READ_MESSAGE_ID, subject);
//}
//
//bool CompilerLogic :: recognizeEmbeddableEval(_CompilerScope& scope, SNode root, ref_t extensionRef, ref_t returningRef, ref_t& subject)
//{
//   return recognizeEmbeddableOp(scope, root, extensionRef, returningRef, EVAL_MESSAGE_ID, subject);
//}
//
//bool CompilerLogic :: recognizeEmbeddableGetAt2(_CompilerScope& scope, SNode root, ref_t extensionRef, ref_t returningRef, ref_t& subject)
//{
//   return recognizeEmbeddableOp2(scope, root, extensionRef, returningRef, READ_MESSAGE_ID, subject);
//}
//
//bool CompilerLogic :: recognizeEmbeddableEval2(_CompilerScope& scope, SNode root, ref_t extensionRef, ref_t returningRef, ref_t& subject)
//{
//   return recognizeEmbeddableOp2(scope, root, extensionRef, returningRef, EVAL_MESSAGE_ID, subject);
//}
//
//bool CompilerLogic :: recognizeEmbeddableIdle(SNode methodNode, bool extensionOne)
//{
//   SNode object = SyntaxTree::findPattern(methodNode, 3,
//      SNodePattern(lxNewFrame),
//      SNodePattern(lxReturning),
//      SNodePattern(extensionOne ? lxLocal : lxThisLocal));
//
//   return extensionOne ? (object == lxLocal && object.argument == -1) : (object == lxThisLocal && object.argument == 1);
//}
//
//bool CompilerLogic :: recognizeEmbeddableMessageCall(SNode methodNode, ref_t& messageRef)
//{
//   SNode attr = methodNode.findChild(lxEmbeddableMssg);
//   if (attr != lxNone) {
//      messageRef = attr.argument;
//
//      return true;
//   }
//   else return false;
//}

bool CompilerLogic :: optimizeEmbeddableGet(_CompilerScope& scope, _Compiler& compiler, SNode node)
{
//   SNode callNode = node.findSubNode(lxDirectCalling, lxSDirctCalling);
//   SNode callTarget = callNode.findChild(lxCallTarget);
//
//   ClassInfo info;
//   if (!defineClassInfo(scope, info, callTarget.argument))
//      return false;
//
//   ref_t subject = info.methodHints.get(Attribute(callNode.argument, maEmbeddableGet));
//   // if it is possible to replace get&subject operation with eval&subject2:local
//   if (subject != 0) {
//      compiler.injectEmbeddableGet(node, callNode, subject);
//
//      return true;
//   }
   /*else */return false;
}

bool CompilerLogic :: optimizeEmbeddableOp(_CompilerScope& scope, _Compiler& compiler, SNode node)
{
//   SNode callNode = node.findSubNode(lxDirectCalling, lxSDirctCalling);
//   SNode callTarget = callNode.findChild(lxCallTarget);
//
//   ClassInfo info;
//   if(!defineClassInfo(scope, info, callTarget.argument))
//      return false;
//
//   for (int i = 0; i < EMBEDDABLEOP_MAX; i++) {
//      EmbeddableOp op = embeddableOps[i];
//      ref_t subject = info.methodHints.get(Attribute(callNode.argument, op.attribute));
//
//      // if it is possible to replace get&subject operation with eval&subject2:local
//      if (subject != 0) {
//         compiler.injectEmbeddableOp(node, callNode, subject, op.paramCount, op.verb);
//
//         return true;
//      }
//   }

   return false;
}

bool CompilerLogic :: validateBoxing(_CompilerScope& scope, _Compiler& compiler, SNode& node, ref_t targetRef, ref_t sourceRef, bool unboxingExpected)
{
   SNode exprNode = node.findSubNodeMask(lxObjectMask);   

   if (targetRef == sourceRef || isCompatible(scope, targetRef, sourceRef)) {
      if (exprNode.type != lxLocalAddress || exprNode.type != lxFieldAddress) {
      }
      else node = lxExpression;
   }
   else if (sourceRef == V_NIL) {
      // NIL reference is never boxed
      node = lxExpression;
   }
   else if (isPrimitiveRef(sourceRef) && (isCompatible(scope, targetRef, resolvePrimitiveReference(scope, sourceRef)) || sourceRef == V_INT32)) {
      //HOTFIX : allowing numeric constant direct boxing
   }
   else if (node.existChild(lxBoxableAttr)) {
      // HOTFIX : if the object was explicitly boxed
   }
   else return false;

   bool localBoxing = false;
   if (exprNode == lxFieldAddress && exprNode.argument > 0) {
      localBoxing = true;
   }
   else if (exprNode == lxFieldAddress && node.argument < 4 && node.argument > 0) {
      localBoxing = true;
   }
   else if (exprNode == lxExternalCall || exprNode == lxStdExternalCall) {
      // the result of external operation should be boxed locally, unboxing is not required (similar to assigning)
      localBoxing = true;
   }

   if (localBoxing) {
      bool unboxingMode = (node == lxUnboxing) || unboxingExpected;

      compiler.injectLocalBoxing(exprNode, node.argument);

      node = unboxingMode ? lxLocalUnboxing : lxBoxing;
   }

   return true;
}

//void CompilerLogic :: injectVariableAssigning(SyntaxWriter& writer, _CompilerScope& scope, _Compiler& compiler, ref_t& targetRef, ref_t& type, int& operand, bool paramMode)
//{
//   ClassInfo info;
//   defineClassInfo(scope, info, targetRef);
//
//   operand = defineStructSize(info, false);
//   
//   if (paramMode) {
//      if (operand == 0) {
//         //HOTFIX : allowing to assign a reference variable
//         // replace the parameter with the field expression
//         compiler.injectFieldExpression(writer);
//      }
//
//      type = info.fieldTypes.get(0).value2;
//      targetRef = info.fieldTypes.get(0).value1;
//   }
//}

bool CompilerLogic :: optimizeEmbeddable(SNode node, _CompilerScope& scope)
{
//   // check if it is a virtual call
//   if (node == lxDirectCalling && getParamCount(node.argument) == 0) {
//      ident_t actionName = scope.module->resolveSubject(getAction(node.argument));
//      if (actionName.find('$') == NOTFOUND_POS)
//         return false;
//
//      SNode callTarget = node.findChild(lxCallTarget);
//
//      ClassInfo info;
//      if (defineClassInfo(scope, info, callTarget.argument) && info.methodHints.get(Attribute(node.argument, maEmbeddableIdle)) == -1) {
//         // if it is an idle call, remove it
//         node = lxExpression;
//
//         return true;
//      }
//   }

   return false;
}

void CompilerLogic :: optimizeBranchingOp(_CompilerScope&, SNode node)
{
   // check if direct comparision with a numeric constant is possible
   SNode ifOp = SyntaxTree::findPattern(node, 3,
      SNodePattern(lxExpression),
      SNodePattern(lxIntOp),
      SNodePattern(lxConstantInt));

   if (ifOp != lxNone) {
      int arg = ifOp.findChild(lxIntValue).argument;

      SNode intOpNode = node.findSubNode(lxIntOp);
      SNode ifNode = node.findChild(lxIf, lxElse);
      if (ifNode != lxNone) {
         SNode trueNode = intOpNode.findChild(ifNode == lxIf ? lxIfValue : lxElseValue);

         if (ifOp.prevNode(lxObjectMask) == lxNone) {
            // if the numeric constant is the first operand
            if (intOpNode.argument == LESS_MESSAGE_ID) {
               intOpNode.argument = GREATER_MESSAGE_ID;
            }
            else if (intOpNode.argument == GREATER_MESSAGE_ID) {
               intOpNode.argument = LESS_MESSAGE_ID;
            }
         }

         if (intOpNode.argument == EQUAL_MESSAGE_ID) {
            if (trueNode.argument == ifNode.argument) {
               ifNode.set(lxIfN, arg);
            }
            else if (trueNode.argument != ifNode.argument) {
               ifNode.set(lxIfNotN, arg);
            }
            else return;
         }
         else if (intOpNode.argument == LESS_MESSAGE_ID) {
            if (trueNode.argument != ifNode.argument) {
               ifNode.set(lxLessN, arg);
            }
            else if (trueNode.argument == ifNode.argument) {
               ifNode.set(lxNotLessN, arg);
            }
            else return;
         }
         else if (intOpNode.argument == GREATER_MESSAGE_ID) {
            if (trueNode.argument != ifNode.argument) {
               ifNode.set(lxGreaterN, arg);
            }
            else if (trueNode.argument == ifNode.argument) {
               ifNode.set(lxNotGreaterN, arg);
            }
            else return;
         }
         else return;

         ifOp = lxIdle;
         intOpNode = lxExpression;
      }
   }
}

//inline void readFirstSignature(ident_t signature, size_t& start, size_t& end, IdentifierString& temp)
//{
//   start = signature.find('$') + 1;
//   end = signature.find(start, '$', getlength(signature));
//   temp.copy(signature.c_str() + start, end - start);
//}
//
//inline void readNextSignature(ident_t signature, size_t& start, size_t& end, IdentifierString& temp)
//{
//   end = signature.find(start, '$', getlength(signature));
//   temp.copy(signature.c_str() + start, end - start);
//}

ref_t CompilerLogic :: resolveMultimethod(_CompilerScope& scope, ref_t multiMessage, ref_t targetRef, ref_t implicitSignatureRef)
{
   if (!implicitSignatureRef || !targetRef)
      return 0;

   //compatible = isSignatureCompatible(scope, signatureRef, signatures);

//   bool openArg = getAbsoluteParamCount(multiMessage) == OPEN_ARG_COUNT;
//   ident_t sour_sign = scope.module->resolveSubject(implicitSignatureRef);

   ref_t signatures[OPEN_ARG_COUNT];
   size_t signatureLen = scope.module->resolveSignature(implicitSignatureRef, signatures);

   ClassInfo info;
   if (defineClassInfo(scope, info, targetRef)) {
      ref_t listRef = info.methodHints.get(Attribute(multiMessage, maOverloadlist));
      if (listRef) {
         _Module* argModule = scope.loadReferenceModule(listRef, listRef);

         _Memory* section = argModule->mapSection(listRef | mskRDataRef, true);
         if (!section || section->Length() < 4)
            return 0;

         MemoryReader reader(section);
         pos_t position = section->Length() - 4;
         while (position != 0) {
            reader.seek(position - 8);
            ref_t argMessage = reader.getDWord();
            ref_t argSign = 0;
            argModule->resolveAction(getAction(argMessage), argSign);

            if (argModule == scope.module) {
               if (isSignatureCompatible(scope, argSign, signatures))
                  return argMessage;
            }
            else {
               if (isSignatureCompatible(scope, argModule, argSign, signatures))
                  return importMessage(argModule, argMessage, scope.module);
            }

            position -= 8;
         }
      }
   }

   return 0;
}

bool CompilerLogic :: validateMessage(ref_t message, bool isClassClass)
{
   bool dispatchOne = getAction(message) == DISPATCH_MESSAGE_ID;

   if (isClassClass && dispatchOne) {
      return false;
   }
   else if (!isClassClass && dispatchOne && getParamCount(message) != 0) {
      return false;
   }
   else return true;
}
