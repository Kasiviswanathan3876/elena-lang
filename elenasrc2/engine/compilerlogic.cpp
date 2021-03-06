//---------------------------------------------------------------------------
//		E L E N A   P r o j e c t:  ELENA Compiler
//
//		This file contains ELENA compiler logic class implementation.
//
//                                              (C)2005-2019, by Alexei Rakov
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

//inline bool isWrappable(int flags)
//{
//   return !test(flags, elWrapper) && test(flags, elSealed);
//}

inline bool isPrimitiveArrayRef(ref_t classRef)
{
   switch (classRef) {
      case V_OBJARRAY:
      case V_INT32ARRAY:
      case V_INT16ARRAY:
      case V_INT8ARRAY:
      case V_BINARYARRAY:
         return true;
      default:
         return false;
   }
}

//inline bool isOpenArgRef(ref_t classRef)
//{
//   return classRef == V_ARGARRAY;
//}
//
//inline ref_t definePrimitiveArrayItem(ref_t classRef)
//{
//   switch (classRef)
//   {
//      case V_INT32ARRAY:
//      case V_INT16ARRAY:
//      case V_INT8ARRAY:
//         return V_INT32;
//      default:
//         return 0;
//   }
//}
//
//inline bool isPrimitiveStructArrayRef(ref_t classRef)
//{
//   switch (classRef)
//   {
//      case V_INT32ARRAY:
//      case V_INT16ARRAY:
//      case V_INT8ARRAY:
//      case V_BINARYARRAY:
//         return true;
//      default:
//         return false;
//   }
//}

inline bool IsInvertedOperator(int& operator_id)
{
   switch (operator_id)
   {
      case NOTEQUAL_OPERATOR_ID:
         operator_id = EQUAL_OPERATOR_ID;

         return true;
      case NOTLESS_OPERATOR_ID:
         operator_id = LESS_OPERATOR_ID;

         return true;
      case NOTGREATER_OPERATOR_ID:
         operator_id = GREATER_OPERATOR_ID;

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

// --- CompilerLogic Optimization Ops ---
struct EmbeddableOp
{
   int attribute;
   int paramCount;   // -1 indicates that operation should be done with the assigning target
   //int verb;

   EmbeddableOp(int attr, int count/*, int verb*/)
   {
      this->attribute = attr;
      this->paramCount = count;
     // this->verb = verb;
   }
};
constexpr auto EMBEDDABLEOP_MAX = /*5*/1;
EmbeddableOp embeddableOps[EMBEDDABLEOP_MAX] =
{
//   EmbeddableOp(maEmbeddableGetAt, 2/*, READ_MESSAGE_ID*/),
//   EmbeddableOp(maEmbeddableGetAt2, 3, READ_MESSAGE_ID),
//   EmbeddableOp(maEmbeddableEval, 2, EVAL_MESSAGE_ID),
//   EmbeddableOp(maEmbeddableEval2, 3, EVAL_MESSAGE_ID),
   EmbeddableOp(maEmbeddableNew, -1/*, 0*/) 
};

// --- CompilerLogic ---

CompilerLogic :: CompilerLogic()
{
   // nil
   operators.add(OperatorInfo(EQUAL_OPERATOR_ID, V_NIL, 0, lxNilOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_OPERATOR_ID, V_NIL, 0, lxNilOp, V_FLAG));
   operators.add(OperatorInfo(EQUAL_OPERATOR_ID, 0, V_NIL, lxNilOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_OPERATOR_ID, 0, V_NIL, lxNilOp, V_FLAG));

   // int32 primitive operations
   operators.add(OperatorInfo(ADD_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(SUB_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(MUL_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(DIV_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(AND_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(OR_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(XOR_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(SHIFTR_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_INT32));
   operators.add(OperatorInfo(SHIFTL_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_INT32));

   operators.add(OperatorInfo(EQUAL_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(LESS_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(NOTLESS_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(GREATER_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(NOTGREATER_OPERATOR_ID, V_INT32, V_INT32, lxIntOp, V_FLAG));

   // subject primitive operations
   operators.add(OperatorInfo(EQUAL_OPERATOR_ID, V_SUBJECT, V_SUBJECT, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_OPERATOR_ID, V_SUBJECT, V_SUBJECT, lxIntOp, V_FLAG));

   // int64 primitive operations
   operators.add(OperatorInfo(ADD_OPERATOR_ID,   V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(SUB_OPERATOR_ID,   V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(MUL_OPERATOR_ID,   V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(DIV_OPERATOR_ID,   V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(AND_OPERATOR_ID,   V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(OR_OPERATOR_ID,    V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(XOR_OPERATOR_ID,   V_INT64, V_INT64, lxLongOp, V_INT64));
   operators.add(OperatorInfo(SHIFTR_OPERATOR_ID,  V_INT64, V_INT32, lxLongOp, V_INT64));
   operators.add(OperatorInfo(SHIFTL_OPERATOR_ID, V_INT64, V_INT32, lxLongOp, V_INT64));

   operators.add(OperatorInfo(EQUAL_OPERATOR_ID, V_INT64, V_INT64, lxLongOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_OPERATOR_ID, V_INT64, V_INT64, lxLongOp, V_FLAG));
   operators.add(OperatorInfo(LESS_OPERATOR_ID, V_INT64, V_INT64, lxLongOp, V_FLAG));
   operators.add(OperatorInfo(NOTLESS_OPERATOR_ID, V_INT64, V_INT64, lxLongOp, V_FLAG));
   operators.add(OperatorInfo(GREATER_OPERATOR_ID, V_INT64, V_INT64, lxLongOp, V_FLAG));
   operators.add(OperatorInfo(NOTGREATER_OPERATOR_ID, V_INT64, V_INT64, lxLongOp, V_FLAG));

   // real64 primitive operations
   operators.add(OperatorInfo(ADD_OPERATOR_ID, V_REAL64, V_REAL64, lxRealOp, V_REAL64));
   operators.add(OperatorInfo(SUB_OPERATOR_ID, V_REAL64, V_REAL64, lxRealOp, V_REAL64));
   operators.add(OperatorInfo(MUL_OPERATOR_ID, V_REAL64, V_REAL64, lxRealOp, V_REAL64));
   operators.add(OperatorInfo(DIV_OPERATOR_ID, V_REAL64, V_REAL64, lxRealOp, V_REAL64));

   operators.add(OperatorInfo(EQUAL_OPERATOR_ID, V_REAL64, V_REAL64, lxRealOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_OPERATOR_ID, V_REAL64, V_REAL64, lxRealOp, V_FLAG));
   operators.add(OperatorInfo(LESS_OPERATOR_ID, V_REAL64, V_REAL64, lxRealOp, V_FLAG));
   operators.add(OperatorInfo(NOTLESS_OPERATOR_ID, V_REAL64, V_REAL64, lxRealOp, V_FLAG));
   operators.add(OperatorInfo(GREATER_OPERATOR_ID, V_REAL64, V_REAL64, lxRealOp, V_FLAG));
   operators.add(OperatorInfo(NOTGREATER_OPERATOR_ID, V_REAL64, V_REAL64, lxRealOp, V_FLAG));

   // array of int32 primitive operations
   operators.add(OperatorInfo(REFER_OPERATOR_ID, V_INT32ARRAY, V_INT32, lxIntArrOp, V_INT32));
   operators.add(OperatorInfo(SET_REFER_OPERATOR_ID, V_INT32ARRAY, V_INT32, V_INT32, lxIntArrOp, 0));
   operators.add(OperatorInfo(SHIFTR_OPERATOR_ID, V_INT32ARRAY, V_INT32, lxIntArrOp, 0));

   // array of int16 primitive operations
   operators.add(OperatorInfo(REFER_OPERATOR_ID, V_INT16ARRAY, V_INT32, lxShortArrOp, V_INT32));
   operators.add(OperatorInfo(SET_REFER_OPERATOR_ID, V_INT16ARRAY, V_INT32, V_INT32, lxShortArrOp, 0));
   operators.add(OperatorInfo(SHIFTR_OPERATOR_ID, V_INT16ARRAY, V_INT32, lxShortArrOp, 0));

   // array of int8 primitive operations
   operators.add(OperatorInfo(REFER_OPERATOR_ID, V_INT8ARRAY, V_INT32, lxByteArrOp, V_INT32));
   operators.add(OperatorInfo(SET_REFER_OPERATOR_ID, V_INT8ARRAY, V_INT32, V_INT32, lxByteArrOp, 0));
   operators.add(OperatorInfo(SHIFTR_OPERATOR_ID, V_INT8ARRAY, V_INT32, lxByteArrOp, 0));

   // array of object primitive operations
   operators.add(OperatorInfo(REFER_OPERATOR_ID, V_OBJARRAY, V_INT32, lxArrOp, V_OBJECT));
   operators.add(OperatorInfo(SET_REFER_OPERATOR_ID, V_OBJARRAY, V_INT32, 0, lxArrOp, 0));
   operators.add(OperatorInfo(SHIFTR_OPERATOR_ID, V_OBJARRAY, V_INT32, lxArrOp, 0));

   // array of structures primitive operations
   operators.add(OperatorInfo(REFER_OPERATOR_ID, V_BINARYARRAY, V_INT32, lxBinArrOp, V_BINARY));
   operators.add(OperatorInfo(SET_REFER_OPERATOR_ID, V_BINARYARRAY, V_INT32, 0, lxBinArrOp, 0));
   operators.add(OperatorInfo(SHIFTR_OPERATOR_ID, V_BINARYARRAY, V_INT32, lxBinArrOp, 0));

   // array of arg list
   operators.add(OperatorInfo(REFER_OPERATOR_ID, V_ARGARRAY, V_INT32, lxArgArrOp, 0));
   operators.add(OperatorInfo(SET_REFER_OPERATOR_ID, V_ARGARRAY, V_INT32, 0, lxArgArrOp, 0));

   //operators.add(OperatorInfo(READ_MESSAGE_ID, V_OBJARRAY, V_INT32, lxArrOp, 0));

   // boolean primitive operations
   operators.add(OperatorInfo(AND_OPERATOR_ID, V_FLAG, V_FLAG, 0, lxBoolOp, V_FLAG));
   operators.add(OperatorInfo(OR_OPERATOR_ID, V_FLAG, V_FLAG, 0, lxBoolOp, V_FLAG));

   // pointer primitive operations
   operators.add(OperatorInfo(EQUAL_OPERATOR_ID, V_PTR32, V_PTR32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_OPERATOR_ID, V_PTR32, V_PTR32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(EQUAL_OPERATOR_ID, V_PTR32, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_OPERATOR_ID, V_PTR32, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(ADD_OPERATOR_ID, V_PTR32, V_INT32, lxIntOp, V_PTR32));
   operators.add(OperatorInfo(SUB_OPERATOR_ID, V_PTR32, V_INT32, lxIntOp, V_PTR32));

   // dword primitive operations
   operators.add(OperatorInfo(EQUAL_OPERATOR_ID, V_DWORD, V_DWORD, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_OPERATOR_ID, V_DWORD, V_DWORD, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(EQUAL_OPERATOR_ID, V_DWORD, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(NOTEQUAL_OPERATOR_ID, V_DWORD, V_INT32, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(LESS_OPERATOR_ID, V_DWORD, V_DWORD, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(NOTLESS_OPERATOR_ID, V_DWORD, V_DWORD, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(GREATER_OPERATOR_ID, V_DWORD, V_DWORD, lxIntOp, V_FLAG));
   operators.add(OperatorInfo(NOTGREATER_OPERATOR_ID, V_DWORD, V_DWORD, lxIntOp, V_FLAG));

}

int CompilerLogic :: checkMethod(ClassInfo& info, ref_t message, ChechMethodInfo& result)
{
   bool methodFound = info.methods.exist(message);

   if (methodFound) {
      int hint = info.methodHints.get(Attribute(message, maHint));
      result.outputReference = info.methodHints.get(Attribute(message, maReference));

      result.embeddable = test(hint, tpEmbeddable);
      result.closure = test(hint, tpAction);
      result.dynamicRequired = test(hint, tpDynamic);

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
   else {
      //HOTFIX : to recognize the predefined messages
      result.outputReference = info.methodHints.get(Attribute(message, maReference));

      //HOTFIX : to recognize the sealed private method call
      //         hint search should be done even if the method is not declared
      return info.methodHints.get(Attribute(message, maHint));
   }
}

int CompilerLogic :: checkMethod(_ModuleScope& scope, ref_t reference, ref_t message, ChechMethodInfo& result)
{
   ClassInfo info;
   result.found = defineClassInfo(scope, info, reference);

   if (result.found) {
      if (test(info.header.flags, elClosed))
         result.directResolved = true;

      if (test(info.header.flags, elWithCustomDispatcher))
         result.withCustomDispatcher = true;

      int hint = checkMethod(info, message, result);
      //if (hint == tpUnknown && test(info.header.flags, elWithArgGenerics)) {
      //   hint = checkMethod(info, overwriteParamCount(message, OPEN_ARG_COUNT), result);
      //   if (hint != tpUnknown) {
      //      result.withOpenArgDispatcher = true;
      //   }
      //   else {
      //      hint = checkMethod(info, overwriteParamCount(message, OPEN_ARG_COUNT + 1), result);
      //      if (hint != tpUnknown) {
      //         result.withOpenArg1Dispatcher = true;
      //      }
      //      else {
      //         hint = checkMethod(info, overwriteParamCount(message, OPEN_ARG_COUNT + 2), result);
      //         if (hint != tpUnknown) {
      //            result.withOpenArg2Dispatcher = true;
      //         }
      //      }
      //   }
      //}

      return hint;
   }
   else return tpUnknown;
}

int CompilerLogic :: resolveCallType(_ModuleScope& scope, ref_t& classReference, ref_t messageRef, ChechMethodInfo& result)
{
   int methodHint = checkMethod(scope, classReference != 0 ? classReference : scope.superReference, messageRef, result);
   int callType = methodHint & tpMask;

   result.stackSafe = test(methodHint, tpStackSafe);

   if (test(messageRef, SPECIAL_MESSAGE)) {
      // HOTFIX : calling closure
      result.closure = true;
   }

   return callType;
}

int CompilerLogic :: resolveOperationType(_ModuleScope& scope, int operatorId, ref_t loperand, ref_t roperand, ref_t& result)
{
   if ((loperand == 0 && roperand != V_NIL) || (roperand == 0 && loperand != V_NIL))
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
         else if (info.roperand == V_NIL) {
            if (roperand == V_NIL) {
               result = info.result;

               return info.operationType;
            }
         }
         else if (info.loperand == V_FLAG && info.roperand == V_FLAG) {
            if (isBoolean(scope, loperand) && isBoolean(scope, roperand)) {
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

int CompilerLogic :: resolveOperationType(_ModuleScope& scope, int operatorId, ref_t loperand, ref_t roperand, ref_t roperand2, ref_t& result)
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

//bool CompilerLogic :: loadBranchingInfo(_CompilerScope& scope, ref_t reference)
//{
//   if (scope.branchingInfo.trueRef == reference || scope.branchingInfo.falseRef == reference)
//      return true;
//
//   ClassInfo info;
//   scope.loadClassInfo(info, reference);
//
//   if ((info.header.flags & elDebugMask) == elEnumList) {
//      _Module* extModule = NULL;
//      _Memory* listSection = NULL;
//
//      while (true) {
//         ref_t listRef = info.statics.get(ENUM_VAR).value1;
//
//         ref_t extRef = 0;
//         extModule = scope.loadReferenceModule(listRef, extRef);
//         listSection = extModule ? extModule->mapSection(extRef | mskRDataRef, true) : NULL;
//
//         if (listSection == NULL && info.header.parentRef != 0) {
//            reference = info.header.parentRef;
//
//            scope.loadClassInfo(info, reference);
//         }
//         else break;
//      }
//
//      if (listSection) {
//         ref_t trueRef = 0;
//         ref_t falseRef = 0;
//
//         // read enum list member and find true / false values
//         _ELENA_::RelocationMap::Iterator it(listSection->getReferences());
//         ref_t currentMask = 0;
//         ref_t memberRef = 0;
//         while (!it.Eof()) {
//            currentMask = it.key() & mskAnyRef;
//            if (currentMask == mskConstantRef) {
//               memberRef = importReference(extModule, it.key() & ~mskAnyRef, scope.module);
//
//               ClassInfo memberInfo;
//               scope.loadClassInfo(memberInfo, memberRef);
//               int attribute = checkMethod(memberInfo, encodeMessage(IF_MESSAGE_ID, 1));
//               if (attribute == (tpIfBranch | tpSealed)) {
//                  trueRef = memberRef;
//               }
//               else if (attribute == (tpIfNotBranch | tpSealed)) {
//                  falseRef = memberRef;
//               }
//            }
//
//            it++;
//         }
//
//         if (trueRef && falseRef) {
//            scope.branchingInfo.reference = reference;
//            scope.branchingInfo.trueRef = trueRef;
//            scope.branchingInfo.falseRef = falseRef;
//
//            return true;
//         }
//      }
//   }
//
//   return false;
//}

bool CompilerLogic :: resolveBranchOperation(_ModuleScope& scope, int operatorId, ref_t loperand, ref_t& reference)
{
   if (!loperand)
      return false;

   if (!isCompatible(scope, scope.branchingInfo.reference, loperand)) {
      return false;
   }

   reference = operatorId == IF_OPERATOR_ID ? scope.branchingInfo.trueRef : scope.branchingInfo.falseRef;

   return true;
}

int CompilerLogic :: resolveNewOperationType(_ModuleScope& scope, ref_t loperand, ref_t roperand)
{
   if (isCompatible(scope, V_INT32, roperand)) {
      ClassInfo info;
      if (scope.loadClassInfo(info, loperand, true)) {
         return test(info.header.flags, elDynamicRole) ? lxNewArrOp : 0;
      }
   }

   return 0;
}

inline bool isPrimitiveCompatible(ref_t targetRef, ref_t sourceRef)
{
   switch (targetRef) {
      case V_PTR32:
      case V_DWORD:
         return sourceRef == V_INT32;
      default:
         return false;
   }
}

bool CompilerLogic :: isCompatible(_ModuleScope& scope, ref_t targetRef, ref_t sourceRef)
{
   if ((!targetRef || targetRef == scope.superReference) && !isPrimitiveRef(sourceRef))
      return true;

   if (sourceRef == V_NIL)
      return true;

   if (isPrimitiveRef(targetRef) && isPrimitiveCompatible(targetRef, sourceRef))
      return true;

   while (sourceRef != 0) {
      if (targetRef != sourceRef) {
         ClassInfo info;
         if (!defineClassInfo(scope, info, sourceRef))
            return false;

         // if it is a structure wrapper
         if (isPrimitiveRef(targetRef) && test(info.header.flags, elWrapper)) {
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

ref_t CompilerLogic :: resolvePrimitive(ClassInfo& info, ref_t& element)
{
   ClassInfo::FieldInfo inner = info.fieldTypes.get(0);
   element = inner.value2;

   return inner.value1;
}

bool CompilerLogic :: isEmbeddableArray(ClassInfo& info)
{
   return test(info.header.flags, elDynamicRole | elStructureRole | elWrapper);
}

bool CompilerLogic :: isVariable(_ModuleScope& scope, ref_t classReference)
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

bool CompilerLogic :: isArray(_ModuleScope& scope, ref_t classReference)
{
   ClassInfo info;
   if (!defineClassInfo(scope, info, classReference))
      return false;

   return isArray(info);
}

bool CompilerLogic :: isArray (ClassInfo& info)
{
   return test(info.header.flags, elDynamicRole);
}

bool CompilerLogic :: isValidType(_ModuleScope& scope, ref_t classReference, bool ignoreUndeclared)
{
   ClassInfo info;
   if (!defineClassInfo(scope, info, classReference))
      return ignoreUndeclared;

   return isValidType(info);
}

bool CompilerLogic :: validateAutoType(_ModuleScope& scope, ref_t& reference)
{
   ClassInfo info;
   if (!defineClassInfo(scope, info, reference))
      return false;

   while (isRole(info)) {
      reference = info.header.parentRef;

      if (!defineClassInfo(scope, info, reference))
         return false;
   }

   return true;
}

bool CompilerLogic :: isValidType(ClassInfo& info)
{
   return !testany(info.header.flags, elRole);
}

bool CompilerLogic :: isEmbeddable(ClassInfo& info)
{
   return test(info.header.flags, elStructureRole) && !test(info.header.flags, elDynamicRole);
}

bool CompilerLogic :: isStacksafeArg(ClassInfo& info)
{
   if (test(info.header.flags, elDynamicRole)) {
      return isEmbeddableArray(info);
   }
   else return isEmbeddable(info);
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
   return test(info.methodHints.get(Attribute(message, maHint)), tpStackSafe);
}

bool CompilerLogic :: isMethodAbstract(ClassInfo& info, ref_t message)
{
   return test(info.methodHints.get(Attribute(message, maHint)), tpAbstract);
}

bool CompilerLogic :: isMethodInternal(ClassInfo& info, ref_t message)
{
   return test(info.methodHints.get(Attribute(message, maHint)), tpInternal);
}

bool CompilerLogic :: isMethodPrivate(ClassInfo& info, ref_t message)
{
   return test(info.methodHints.get(Attribute(message, maHint)), tpPrivate);
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

//bool CompilerLogic :: isDispatcher(ClassInfo& info, ref_t message)
//{
//   return (info.methodHints.get(Attribute(message, maHint)) & tpMask) == tpDispatcher;
//}

inline ident_t resolveActionName(_Module* module, ref_t message)
{
   ref_t signRef = 0;
   return module->resolveAction(getAction(message), signRef);
}

void CompilerLogic :: injectOverloadList(_ModuleScope& scope, ClassInfo& info, _Compiler& compiler, ref_t classRef)
{
   for (auto it = info.methods.start(); !it.Eof(); it++) {
      if (*it && isMultiMethod(info, it.key())) {
         ref_t message = it.key();

         // create a new overload list
         ref_t listRef = scope.mapAnonymous(resolveActionName(scope.module, message));

         info.methodHints.exclude(Attribute(message, maOverloadlist));
         info.methodHints.add(Attribute(message, maOverloadlist), listRef);
         if (test(message, STATIC_MESSAGE)) {
            info.methodHints.exclude(Attribute(message & ~STATIC_MESSAGE, maOverloadlist));
            info.methodHints.add(Attribute(message & ~STATIC_MESSAGE, maOverloadlist), listRef);
         }

         // sort the overloadlist
         int defaultOne[0x20];
         int* list = defaultOne;
         size_t capcity = 0x20;
         size_t len = 0;
         for (auto h_it = info.methodHints.start(); !h_it.Eof(); h_it++) {
            if (h_it.key().value2 == maMultimethod && *h_it == message) {
               if (len == capcity) {
                  int* new_list = new int[capcity + 0x10];
                  memmove(new_list, list, capcity * 4);
                  list = new_list;
                  capcity += 0x10;
               }

               ref_t omsg = h_it.key().value1;
               list[len] = omsg;
               for (size_t i = 0; i < len; i++) {
                  if (isSignatureCompatible(scope, omsg, list[i])) {
                     memmove(list + (i + 1) * 4, list + i * 4, (len - i) * 4);
                     list[i] = omsg;
                     break;
                  }
               }
               len++;
            }
         }

         // fill the overloadlist
         for (size_t i = 0; i < len; i++) {
            if (test(info.header.flags, elSealed)/* || test(message, SEALED_MESSAGE)*/) {
               compiler.generateSealedOverloadListMember(scope, listRef, list[i], classRef);
            }
            else if (test(info.header.flags, elClosed)) {
               compiler.generateClosedOverloadListMember(scope, listRef, list[i], classRef);
            }
            else compiler.generateOverloadListMember(scope, listRef, list[i]);
         }

         if (capcity > 0x20)
            delete[] list;
      }
   }
}

//void CompilerLogic :: injectVirtualFields(_CompilerScope& scope, SNode node, ref_t classRef, ClassInfo& info, _Compiler& compiler)
//{
//   // generate enumeration list static field
//   if ((info.header.flags & elDebugMask) == elEnumList && !test(info.header.flags, elNestedClass)) {
//      compiler.injectVirtualStaticConstField(scope, node, ENUM_VAR, classRef);
//   }
//}

void CompilerLogic :: injectVirtualCode(_ModuleScope& scope, SNode node, ref_t classRef, ClassInfo& info, _Compiler& compiler, bool closed)
{
//   // generate enumeration list
//   if ((info.header.flags & elDebugMask) == elEnumList && test(info.header.flags, elNestedClass)) {
//      ClassInfo::FieldInfo valuesField = info.statics.get(ENUM_VAR);
//
//      compiler.generateListMember(scope, valuesField.value1, classRef);
//   }

   if (!testany(info.header.flags, elClassClass | elNestedClass) && classRef != scope.superReference && !closed && !test(info.header.flags, elExtension)) {
      // auto generate cast$<type> message for explicitly declared classes
      ref_t signRef = scope.module->mapSignature(&classRef, 1, false);
      ref_t actionRef = scope.module->mapAction(CAST_MESSAGE, signRef, false);

      compiler.injectVirtualReturningMethod(scope, node, encodeAction(actionRef), SELF_VAR, classRef);
   }

   // generate structure embeddable constructor
   if (test(info.header.flags, /*elSealed | */elStructureRole)) {
      List<ref_t/*Attribute*/> generatedConstructors;
      bool found = 0;
      SNode current = node.firstChild();
      while (current != lxNone) {
         if (current == lxConstructor) {
            SNode attr = current.firstChild();
            while (attr != lxNone) {
               if (attr == lxAttribute && attr.argument == tpEmbeddable) {
//                  SNode multiMethodAttr = current.findChild(lxMultiMethodAttr);

                  ref_t dummy = 0;
                  ident_t actionName = scope.module->resolveAction(getAction(current.argument), dummy);
                  if (actionName.compare(CONSTRUCTOR_MESSAGE)) {
                     // NOTE : Only implciit constructors can be embeddable
                     generatedConstructors.add(/*Attribute(*/current.argument/*, multiMethodAttr.argument)*/);

                     current.set(lxClassMethod, current.argument | STATIC_MESSAGE);
                     attr.argument = tpPrivate;

                     //                  if (multiMethodAttr != lxNone)
                     //                     // HOTFIX : replace the multimethod message with correct one
                     //                     multiMethodAttr.setArgument(multiMethodAttr.argument | SEALED_MESSAGE);

                     found = true;
                  }
                  break;
               }
               attr = attr.nextNode();
            }
         }
         current = current.nextNode();
      }

      if (found) {
         for (auto it = generatedConstructors.start(); !it.Eof(); it++) {
            ref_t message = /*(*/*it/*).value1*/;
//            ref_t dispatchArg = (*it).value2;

            compiler.injectEmbeddableConstructor(node, message, message | STATIC_MESSAGE/*, dispatchArg*/);
         }
      }
   }
}

void CompilerLogic :: injectVirtualMultimethods(_ModuleScope& scope, SNode node, _Compiler& compiler, List<ref_t>& implicitMultimethods, LexicalType methodType)
{
   // generate implicit mutli methods
   for (auto it = implicitMultimethods.start(); !it.Eof(); it++) {
      ref_t message = *it;

      if (methodType == lxConstructor && getParamCount(message) == 1 
         && getAction(message) == getAction(scope.constructor_message)) 
      {
         // HOTFIX : implicit multi-method should be compiled differently
         compiler.injectVirtualMultimethodConversion(scope, node, *it, methodType);
      }
      else compiler.injectVirtualMultimethod(scope, node, *it, methodType);
   }
}

bool isEmbeddableDispatcher(_ModuleScope& scope, SNode current)
{
   SNode attr = current.firstChild();
   bool embeddable = false;
   bool implicit = true;
   while (attr != lxNone) {
      if (attr == lxAttribute) {
         switch (attr.argument) {
            case V_EMBEDDABLE:
            {
               SNode dispatch = current.findChild(lxDispatchCode);

               embeddable = isSingleStatement(dispatch);
               break;
            }
            case V_METHOD:
            case V_CONSTRUCTOR:
            case V_DISPATCHER:
               implicit = false;
               break;
         }
      }
      else if (attr == lxNameAttr && embeddable && implicit) {
         if (scope.attributes.get(attr.firstChild(lxTerminalMask).identifier()) == V_DISPATCHER) {
            return true;
         }
         else break;
      }

      attr = attr.nextNode();
   }

   return false;
}

bool CompilerLogic :: isWithEmbeddableDispatcher(_ModuleScope& scope, SNode node)
{
   SNode current = node.firstChild();
   while (current != lxNone) {
      if (current == lxClassMethod && current.existChild(lxDispatchCode)) {
         if (isEmbeddableDispatcher(scope, current)) {
            return true;
         }
      }
      current = current.nextNode();
   }

   return false;
}

void CompilerLogic :: injectInterfaceDisaptch(_ModuleScope& scope, _Compiler& compiler, SNode node, ref_t parentRef)
{
   SNode current = node.firstChild();
   SNode dispatchMethodNode;
   SNode dispatchNode;
   while (current != lxNone) {
      if (current == lxClassMethod && current.existChild(lxDispatchCode)) {
         if (isEmbeddableDispatcher(scope, current)) {
            dispatchMethodNode = current;
            dispatchNode = current.findChild(lxDispatchCode).firstChild();
         }

      }
      current = current.nextNode();
   }

   ClassInfo info;
   scope.loadClassInfo(info, parentRef);
   for (auto it = info.methodHints.start(); !it.Eof(); it++) {
      if (it.key().value2 == maHint && test(*it, tpAbstract)) {
         compiler.injectVirtualDispatchMethod(node, it.key().value1, dispatchNode.type, dispatchNode.identifier());
      }
   }

   dispatchMethodNode = lxIdle;
}

void CompilerLogic :: verifyMultimethods(_ModuleScope& scope, SNode node, ClassInfo& info, List<ref_t>& implicitMultimethods)
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
         ref_t multiMethod = info.methodHints.get(Attribute(current.argument, maMultimethod));
         if (multiMethod != lxNone) {
            ref_t outputRefMulti = info.methodHints.get(Attribute(multiMethod, maReference));
            if (outputRefMulti != 0) {
               ref_t outputRef = info.methodHints.get(Attribute(current.argument, maReference));
               if (outputRef == 0) {
                  scope.raiseError(errNotCompatibleMulti, findSourceRef(current), current.findChild(lxNameAttr).firstChild(lxTerminalMask));
               }
               else if (!isCompatible(scope, outputRefMulti, outputRef)) {
                  scope.raiseError(errNotCompatibleMulti, findSourceRef(current), current.findChild(lxNameAttr).firstChild(lxTerminalMask));
               }
            }            
         }
      }
      current = current.nextNode();
   }
}

bool CompilerLogic :: isBoolean(_ModuleScope& scope, ref_t reference)
{
   return isCompatible(scope, scope.branchingInfo.reference, reference);
}

void CompilerLogic :: injectOperation(SyntaxWriter& writer, _ModuleScope& scope, int operator_id, int operationType, ref_t& reference, ref_t elementRef)
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
         resolveBranchOperation(scope, IF_OPERATOR_ID, scope.branchingInfo.reference, dummy);
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

//bool CompilerLogic :: injectImplicitCreation(SyntaxWriter& writer, _CompilerScope& scope, _Compiler& compiler, ref_t targetRef)
//{
//   ClassInfo info;
//   if (!defineClassInfo(scope, info, targetRef))
//      return false;
//
//   if (test(info.header.flags, elStateless))
//      return false;
//
//   ref_t implicitConstructor = encodeMessage(DEFAULT_MESSAGE_ID, 0) | SPECIAL_MESSAGE;
//   if (!info.methods.exist(implicitConstructor, true))
//      return injectDefaultCreation(writer, scope, compiler, targetRef, info.header.classRef);
//
//   bool stackSafe = isMethodStacksafe(info, implicitConstructor);
//
//   if (test(info.header.flags, elStructureRole)) {
//      compiler.injectConverting(writer, lxDirectCalling, implicitConstructor, lxCreatingStruct, info.size, targetRef, targetRef, stackSafe);
//      //compiler.injectConverting(writer, lxNone, 0, lxImplicitCall, encodeAction(NEWOBJECT_MESSAGE_ID), info.header.classRef, targetRef, stackSafe);
//
//      return true;
//   }
//   else if (test(info.header.flags, elDynamicRole)) {
//      return false;
//   }
//   else {
//      compiler.injectConverting(writer, lxDirectCalling, implicitConstructor, lxCreatingClass, info.fields.Count(), targetRef, targetRef, stackSafe);
//      //compiler.injectConverting(writer, lxNone, 0, lxImplicitCall, encodeAction(NEWOBJECT_MESSAGE_ID), info.header.classRef, targetRef, stackSafe);
//
//      return true;
//   }
//}

inline ref_t getSignature(_ModuleScope& scope, ref_t message)
{
   ref_t actionRef, dummy;
   int paramCount;
   decodeMessage(message, actionRef, paramCount, dummy);

   ref_t signRef = 0;
   scope.module->resolveAction(actionRef, signRef);

   return signRef;
}

bool CompilerLogic :: isSignatureCompatible(_ModuleScope& scope, ref_t targetMessage, ref_t sourceMessage)
{
   ref_t sourceSignatures[ARG_COUNT];
/*   size_t len = */scope.module->resolveSignature(getSignature(scope, sourceMessage), sourceSignatures);

   return isSignatureCompatible(scope, getSignature(scope, targetMessage), sourceSignatures);
}

bool CompilerLogic :: isSignatureCompatible(_ModuleScope& scope, ref_t targetSignature, ref_t* sourceSignatures)
{
   ref_t targetSignatures[ARG_COUNT];
   size_t len = scope.module->resolveSignature(targetSignature, targetSignatures);
   if (len < 0)
      return false;

   for (size_t i = 0; i < len; i++) {
      if (!isCompatible(scope, targetSignatures[i], sourceSignatures[i]))
         return false;
   }

   return true;
}

bool CompilerLogic :: isSignatureCompatible(_ModuleScope& scope, _Module* targetModule, ref_t targetSignature, ref_t* sourceSignatures)
{
   ref_t targetSignatures[ARG_COUNT];
   size_t len = targetModule->resolveSignature(targetSignature, targetSignatures);

   for (size_t i = 0; i < len; i++) {
      if (!isCompatible(scope, importReference(targetModule, targetSignatures[i], scope.module), sourceSignatures[i]))
         return false;
   }

   return true;

}

void CompilerLogic :: setSignatureStacksafe(_ModuleScope& scope, ref_t targetSignature, int& stackSafeAttr)
{
   ref_t targetSignatures[ARG_COUNT];
   size_t len = scope.module->resolveSignature(targetSignature, targetSignatures);
   if (len <= 0)
      return;

   int flag = 1;
   for (size_t i = 0; i < len; i++) {
      flag <<= 1;

      if (isStacksafeArg(scope, targetSignatures[i]))
         stackSafeAttr |= flag;
   }
}

void CompilerLogic :: setSignatureStacksafe(_ModuleScope& scope, _Module* targetModule, ref_t targetSignature, int& stackSafeAttr)
{
   ref_t targetSignatures[ARG_COUNT];
   size_t len = targetModule->resolveSignature(targetSignature, targetSignatures);
   if (len <= 0)
      return;

   int flag = 1;
   for (size_t i = 0; i < len; i++) {
      flag <<= 1;

      if (isStacksafeArg(scope, importReference(targetModule, targetSignatures[i], scope.module)))
         stackSafeAttr |= flag;
   }
}

bool CompilerLogic :: injectImplicitConstructor(SyntaxWriter& writer, _ModuleScope& scope, _Compiler& compiler, ClassInfo&/* info*/, ref_t targetRef/*, ref_t elementRef*/, ref_t* signatures, int paramCount)
{
   ref_t signRef = scope.module->mapSignature(signatures, paramCount, false);

   int stackSafeAttr = 0;
   ref_t messageRef = resolveImplicitConstructor(scope, targetRef, signRef, paramCount, stackSafeAttr);
   if (messageRef) {
      compiler.injectConverting(writer, lxDirectCalling, messageRef, lxClassSymbol, targetRef, getClassClassRef(scope, targetRef), stackSafeAttr);

      return true;

   }
   else return false;
}

bool CompilerLogic :: injectConstantConstructor(SyntaxWriter& writer, _ModuleScope& scope, _Compiler& compiler, ref_t targetRef, ref_t messageRef)
{
   int stackSafeAttr = 0;
   setSignatureStacksafe(scope, scope.literalReference, stackSafeAttr);

   compiler.injectConverting(writer, lxDirectCalling, messageRef, lxClassSymbol, targetRef, getClassClassRef(scope, targetRef), stackSafeAttr);

   return true;
}

ref_t CompilerLogic :: getClassClassRef(_ModuleScope& scope, ref_t targetRef)
{
   ClassInfo info;
   if (!defineClassInfo(scope, info, targetRef, true))
      return 0;

   return info.header.classRef;
}

ref_t CompilerLogic :: resolveImplicitConstructor(_ModuleScope& scope, ref_t targetRef, ref_t signRef, int paramCount, int& stackSafeAttr)
{
   ref_t classClassRef = getClassClassRef(scope, targetRef);
   ref_t messageRef = encodeMessage(scope.module->mapAction(CONSTRUCTOR_MESSAGE, 0, false), paramCount, 0);
   if (signRef != 0) {
      // try to resolve implicit multi-method
      ref_t resolvedMessage = resolveMultimethod(scope, messageRef, classClassRef, signRef, stackSafeAttr);
      if (resolvedMessage)
         return resolvedMessage;
   }

   ClassInfo classClassinfo;
   if (!defineClassInfo(scope, classClassinfo, classClassRef))
      return 0;

   if (classClassinfo.methods.exist(messageRef)) {
      return messageRef;
   }
   else return 0;
}

bool CompilerLogic :: injectImplicitConversion(SyntaxWriter& writer, _ModuleScope& scope, _Compiler& compiler, ref_t targetRef, ref_t sourceRef, 
   ref_t elementRef, ident_t ns)
{
//   if (targetRef == 0 && isPrimitiveRef(sourceRef)) {
//      if (isPrimitiveArrayRef(sourceRef)) {
//         // HOTFIX : replace generic object with a generic array
//         targetRef = scope.arrayReference;
//      }
//      else if (sourceRef == V_INT32) {
//         // HOTFIX : replace generic object with an integer constant
//         targetRef = scope.intReference;
//      }
//      else if (sourceRef == V_INT64) {
//         // HOTFIX : replace generic object with an integer constant
//         targetRef = scope.longReference;
//      }
//      else if (sourceRef == V_REAL64) {
//         // HOTFIX : replace generic object with an integer constant
//         targetRef = scope.realReference;
//      }
//      else if (sourceRef == V_MESSAGE) {
//         targetRef = scope.messageReference;
//      }
//      else if (sourceRef == V_SIGNATURE) {
//         targetRef = scope.signatureReference;
//      }
//   }

   ClassInfo info;
   if (!defineClassInfo(scope, info, targetRef))
      return false;

   // if the target class is wrapper around the source
   if (test(info.header.flags, elWrapper) && !test(info.header.flags, elDynamicRole)) {
      ClassInfo::FieldInfo inner = info.fieldTypes.get(0);

      bool compatible = false;
      if (test(info.header.flags, elStructureWrapper)) {
         if (isPrimitiveRef(sourceRef)) {
            compatible = isCompatible(scope, inner.value1, sourceRef);
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

//   // HOTFIX : trying to typecast primitive structure array
//   if (isPrimitiveStructArrayRef(sourceRef) && test(info.header.flags, elStructureRole | elDynamicRole)) {
//      ClassInfo sourceInfo;      
//      if (sourceRef == V_BINARYARRAY && elementRef != 0) {
//         if (defineClassInfo(scope, sourceInfo, elementRef, true)) {
//            if (-sourceInfo.size == info.size && isCompatible(scope, elementRef, info.fieldTypes.get(-1).value1)) {
//               compiler.injectBoxing(writer, scope,
//                  test(info.header.flags, elReadOnlyRole) ? lxBoxing : lxUnboxing, info.size, targetRef);
//
//               return true;
//            }
//         }
//      }
//      else {
//         if (defineClassInfo(scope, sourceInfo, sourceRef, true)) {
//            if (sourceInfo.size == info.size && isCompatible(scope, definePrimitiveArrayItem(sourceRef), info.fieldTypes.get(-1).value1)) {
//               compiler.injectBoxing(writer, scope,
//                  test(info.header.flags, elReadOnlyRole) ? lxBoxing : lxUnboxing, info.size, targetRef);
//
//               return true;
//            }
//         }
//      }
//   }

   // COMPILE MAGIC : trying to typecast primitive array
   if (isPrimitiveArrayRef(sourceRef) && test(info.header.flags, elDynamicRole/* | elWrapper*/)) {
      ref_t boxingArg = isEmbeddable(scope, elementRef) ? - 1 : 0;

      if (isCompatible(scope, info.fieldTypes.get(-1).value2, elementRef)) {
         compiler.injectBoxing(writer, scope,
            test(info.header.flags, elReadOnlyRole) ? lxBoxing : lxUnboxing, boxingArg, targetRef, true);

         return true;
      }
   }

   // COMPILE MAGIC : trying to typecast wrapper
   if (sourceRef == V_WRAPPER && isCompatible(scope, targetRef, elementRef)) {
      compiler.injectBoxing(writer, scope,
         isReadonly(info) ? lxBoxing : lxUnboxing,
         test(info.header.flags, elStructureRole) ? info.size : 0, targetRef);

      return true;
   }

//   // HOTFIX : trying to typecast open argument list
//   if (isOpenArgRef(sourceRef) && test(info.header.flags, elDynamicRole | elNonStructureRole)) {
//      if (isCompatible(scope, info.fieldTypes.get(-1).value1, elementRef)) {
//         compiler.injectBoxing(writer, scope, lxArgBoxing, 0, targetRef, true);
//
//         return true;
//      }
//   }

//   // check if there are implicit constructors
//   if (sourceRef == V_OBJARRAY) {
//      // HOTFIX : recognize primitive object array
//      sourceRef = firstNonZero(scope.arrayReference, scope.superReference);
//
//      compiler.injectBoxing(writer, scope,
//         test(info.header.flags, elReadOnlyRole) ? lxBoxing : lxUnboxing, 0, sourceRef, true);
//   }
   // HOTFIX : recognize primitive data except of a constant literal
   /*else */if (isPrimitiveRef(sourceRef) && sourceRef != V_STRCONSTANT)
      sourceRef = compiler.resolvePrimitiveReference(scope, sourceRef, elementRef, ns, false);

   return injectImplicitConstructor(writer, scope, compiler, info, targetRef, /*elementRef, */&sourceRef, 1);
}

void CompilerLogic :: injectNewOperation(SyntaxWriter& writer, _ModuleScope& scope, int operation, ref_t targetRef, ref_t elementRef)
{
   int size = defineStructSize(scope, targetRef, elementRef);
   if (size != 0)
      writer.appendNode(lxSize, size);

   writer.insert((LexicalType)operation, targetRef);
   writer.closeNode();
}

bool CompilerLogic :: defineClassInfo(_ModuleScope& scope, ClassInfo& info, ref_t reference, bool headerOnly)
{
   if (isPrimitiveRef(reference) && !headerOnly) {
      scope.loadClassInfo(info, scope.superReference);
   }

   switch (reference)
   {
      case V_INT32:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugDWORD | elStructureRole | elReadOnlyRole;
         info.size = 4;
         break;
      case V_INT64:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugQWORD | elStructureRole | elReadOnlyRole;
         info.size = 8;
         break;
      case V_REAL64:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugReal64 | elStructureRole | elReadOnlyRole;
         info.size = 8;
         break;
      case V_PTR32:
         info.header.parentRef = scope.superReference;
         info.header.flags = elStructureRole;
         info.size = 4;
         break;
      case V_DWORD:
         info.header.parentRef = scope.superReference;
         info.header.flags = elStructureRole | elReadOnlyRole;
         info.size = 4;
         break;
      case V_SUBJECT:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugSubject | elStructureRole | elReadOnlyRole;
         info.size = 4;
         break;
      case V_MESSAGE:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugMessage | elStructureRole | elReadOnlyRole;
         info.size = 4;
         break;
      case V_EXTMESSAGE:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugMessage | elStructureRole | elReadOnlyRole;
         info.size = 8;
         break;
      case V_SYMBOL:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugReference | elStructureRole | elReadOnlyRole;
         info.size = 4;
         break;
      case V_INT32ARRAY:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugIntegers | elStructureRole | elDynamicRole | elWrapper;
         info.size = -4;
         break;
      case V_INT16ARRAY:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugShorts | elStructureRole | elDynamicRole | elWrapper;
         info.size = -2;
         break;
      case V_INT8ARRAY:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugBytes | elStructureRole | elDynamicRole | elWrapper;
         info.size = -1;
         break;
      case V_OBJARRAY:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDebugArray | elDynamicRole;
         info.size = 0;
         break;
      case V_BINARYARRAY:
         info.header.parentRef = scope.superReference;
         info.header.flags = elDynamicRole | elStructureRole | elWrapper;
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
            //info.size = 0;
         }
         break;
   }

   return true;
}

int CompilerLogic :: defineStructSizeVariable(_ModuleScope& scope, ref_t reference, ref_t elementRef, bool& variable)
{
   if (reference == V_BINARYARRAY && elementRef != 0) {
      variable = true;

      return -defineStructSizeVariable(scope, elementRef, 0, variable);
   }
   else if (reference == V_WRAPPER && elementRef != 0) {
      return defineStructSizeVariable(scope, elementRef, 0, variable);
   }
   else if (reference == V_OBJARRAY && elementRef != 0) {
      return -defineStructSizeVariable(scope, elementRef, 0, variable);
   }
   else if (reference == V_INT32ARRAY) {
      variable = true;

      return -4;
   }
   else if (reference == V_INT16ARRAY) {
      variable = true;

      return -2;
   }
   else if (reference == V_INT8ARRAY) {
      variable = true;

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
   else if (isEmbeddableArray(info)) {
      return info.size;
   }

   return 0;
}

void CompilerLogic :: tweakClassFlags(_ModuleScope& scope, _Compiler& compiler, ref_t classRef, ClassInfo& info, bool classClassMode)
{
   if (classClassMode) {
      // class class is always stateless and final
      info.header.flags |= elStateless;
      info.header.flags |= elFinal;
   }

   if (test(info.header.flags, elNestedClass)) {
      // stateless inline class
      if (info.fields.Count() == 0 && !test(info.header.flags, elStructureRole)) {
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

//   // verify if the class may be a wrapper
//   if (isWrappable(info.header.flags) && info.fields.Count() == 1 &&
//      test(info.methodHints.get(Attribute(encodeAction(DISPATCH_MESSAGE_ID), maHint)), tpEmbeddable))
//   {
//      if (test(info.header.flags, elStructureRole)) {
//         ClassInfo::FieldInfo field = *info.fieldTypes.start();
//
//         ClassInfo fieldInfo;
//         if (defineClassInfo(scope, fieldInfo, field.value1, true) && isEmbeddable(fieldInfo)) {
//            // wrapper around embeddable object should be marked as embeddable wrapper
//            info.header.flags |= elEmbeddableWrapper;
//
//            if ((info.header.flags & elDebugMask) == 0)
//               info.header.flags |= fieldInfo.header.flags & elDebugMask;
//         }
//      }
//      else info.header.flags |= elWrapper;
//   }

   if (test(info.header.flags, elDynamicRole | elStructureRole)) {
      if (classRef == scope.literalReference) {
         // recognize string constant
         if (info.size == -1) {
            info.header.flags |= elDebugLiteral;
         }
      }
      else if (classRef == scope.wideReference) {
         // recognize wide string constant
         if (info.size == -2) {
            info.header.flags |= elDebugWideLiteral;
         }
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
         switch (itemRef) {
            case V_INT32ARRAY:
               info.header.flags |= elDebugIntegers;
               break;
            case V_INT16ARRAY:
               info.header.flags |= elDebugShorts;
               break;
            case V_INT8ARRAY:
               info.header.flags |= elDebugBytes;
               break;
            default:
               info.header.flags |= elDebugBytes;
               break;
         }
      }
   }

   // adjust objects with custom dispatch handler
   if (info.methods.exist(scope.dispatch_message, true) && classRef != scope.superReference) {
      info.header.flags |= elWithCustomDispatcher;
   }

   // generate operation list if required
   injectOverloadList(scope, info, compiler, classRef);
}

bool CompilerLogic :: validateArgumentAttribute(int attrValue, bool& byRefArg, bool& paramsArg)
{
   switch ((size_t)attrValue) {
      case V_WRAPPER:
         if (!byRefArg) {
            byRefArg = true;
            return true;
         }
         else return false;
      case V_ARGARRAY:
         if (!paramsArg) {
            paramsArg = true;
            return true;
         }
         else return false;
      case V_VARIABLE:
         return true;
   }
   return false;
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
      case V_LIMITED:
         attrValue = (elClosed | elAbstract | elNoCustomDispatcher);
         return true;
      case V_CLOSED:
         attrValue = elClosed;
         return true;
      case V_STRUCT:
         attrValue = elStructureRole;
         return true;
//      case V_ENUMLIST:
//         attrValue = elAbstract | elEnumList | elClosed;
//         return true;
////      case V_DYNAMIC:
////         attrValue = elDynamicRole;
////         return true;
      case V_CONST:
         attrValue = elReadOnlyRole;
         return true;
      case V_EXTENSION:
         attrValue = elExtension;
         return true;
      case V_NOSTRUCT:
         attrValue = elNonStructureRole;
         return true;
      case V_GROUP:
         attrValue = elGroup;
         return true;
      //case V_TAPEGROUP:
      //   attrValue = elTapeGroup;
//         return true;
      case V_CLASS:
      case V_PUBLIC:
      case V_INTERNAL:
         attrValue = 0;
         return true;
      case V_SINGLETON:
         attrValue = elRole | elNestedClass;
         return true;
      default:
         return false;
   }
}

bool CompilerLogic :: validateImplicitMethodAttribute(int& attrValue)
{
   bool dummy = false;
   switch ((size_t)attrValue)
   {
      case V_METHOD:
      case V_CONSTRUCTOR:
      case V_DISPATCHER:
      case V_CONVERSION:
      case V_GENERIC:
      case V_ACTION:
         return validateMethodAttribute(attrValue, dummy);
      default:
         return false;
   }
}

bool CompilerLogic :: validateMethodAttribute(int& attrValue, bool& explicitMode)
{
   switch ((size_t)attrValue)
   {
      case V_EMBEDDABLE:
         attrValue = tpEmbeddable;
         return true;
      case V_GENERIC:
         attrValue = (tpGeneric | tpSealed);
         return true;
      case V_PRIVATE:
         attrValue = (tpPrivate | tpSealed);
         return true;
//      case V_PUBLIC:
//         attrValue = 0;
//         return true;
      case V_INTERNAL:
         attrValue = tpInternal;
         return true;
      case V_SEALED:
         attrValue = tpSealed;
         return true;
      case V_ACTION:
         attrValue = tpAction;
         explicitMode = true;
         return true;
      case V_CONSTRUCTOR:
         attrValue = tpConstructor;
         explicitMode = true;
         return true;
      case V_CONVERSION:
         attrValue = (tpConversion | tpSealed);
         return true;
      case V_INITIALIZER:
         attrValue = (tpSpecial | tpPrivate | tpInitializer);
         return true;
      case V_METHOD:
         attrValue = 0;
         explicitMode = true;
         return true;
      case V_STATIC:
         attrValue = (tpStatic | tpSealed);
         return true;
//      case V_SET:
//         attrValue = tpAccessor;
//         return true;
      case V_ABSTRACT:
         attrValue = tpAbstract;
         return true;
      case V_PREDEFINED:
         attrValue = tpPredefined;
         return true;
      case V_DISPATCHER:
         attrValue = tpDispatcher;
         explicitMode = true;
         return true;
//      case V_STACKUNSAFE:
//         attrValue = tpDynamic;
//         return true;
      case V_GETACCESSOR:
         attrValue = tpGetAccessor;
         return true;
      case V_SETACCESSOR:
         attrValue = tpSetAccessor;
         return true;
      default:
         return false;
   }
}

bool CompilerLogic :: validateFieldAttribute(int& attrValue, bool& isSealed, bool& isConstant, bool& isEmbeddable)
{
   switch ((size_t)attrValue)
   {
      case V_EMBEDDABLE:
         if (!isEmbeddable) {
            isEmbeddable = true;
            attrValue = -1;
            return true;
         }
         else return false;
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
      case V_FLOAT:
      case V_BINARY:
      case V_INTBINARY:
      case V_PTRBINARY:
      case V_STRING:
      case V_MESSAGE:
      case V_SUBJECT:
      case V_EXTMESSAGE:
      case V_SYMBOL:
         attrValue = 0;
         return true;
      case V_FIELD:
         attrValue = -1;
         return true;
      default:
         return false;
   }
}

bool CompilerLogic :: validateExpressionAttribute(ref_t attrValue, ExpressionAttributes& attributes)
{
   switch (attrValue) {
      case 0:
         // HOTFIX : recognize idle attributes
         return true;
      case V_VARIABLE:
      case V_AUTO:
         attributes.typeAttr = true;
         return true;
      case V_CONVERSION:
         attributes.castAttr = true;
         return true;
      case V_NEWOP:
         attributes.newOpAttr = true;
         return true;
      case V_FORWARD:
         attributes.forwardAttr = true;
         return true;
      case V_EXTERN:
         attributes.externAttr = true;
         return true;
      case V_WRAPPER:
         attributes.refAttr = true;
         return true;
	  case V_ARGARRAY:
			attributes.paramsAttr = true;
			return true;
	  case V_INTERN:
         attributes.internAttr = true;
         return true;
      case V_LOOP:
         attributes.loopAttr = true;
         return true;
      case V_MEMBER:
         attributes.memberAttr = true;
         return true;
      case V_SUBJECT:
         attributes.subjAttr = true;
         return true;
      case V_MESSAGE:
         attributes.mssgAttr = true;
         return true;
      case V_GROUP:
         attributes.wrapAttr = true;
         return true;
      case V_CLASS:
         attributes.classAttr = true;
         return true;
      case V_DIRECT:
         attributes.directAttr = true;
         return true;
      case V_LAZY:
         attributes.lazyAttr = true;
         return true;
      default:
         return false;
   }
}

bool CompilerLogic :: validateSymbolAttribute(int attrValue, bool& constant, bool& staticOne, bool& preloadedOne)
{
   if (attrValue == (int)V_CONST) {
      constant = true;

      return true;
   }
   else if (attrValue == (int)V_SYMBOLEXPR) {
      return true;
   }
   else if (attrValue == (int)V_STATIC) {
      staticOne = true;

      return true;
   }
   else if (attrValue == (int)V_PRELOADED) {
      preloadedOne = true;

      return true;
   }
   else if (attrValue == (int)V_PUBLIC || attrValue == (int)V_INTERNAL) {
      return true;
   }
   else return false;
}

//////bool CompilerLogic :: validateWarningAttribute(int& attrValue)
//////{
//////   switch ((size_t)attrValue)
//////   {
//////      case V_WARNING1:

//////         attrValue = WARNING_MASK_0;
//////         return true;
//////      case V_WARNING2:
//////         attrValue = WARNING_MASK_1;
//////         return true;
//////      case V_WARNING3:
//////         attrValue = WARNING_MASK_2;
//////         return true;
//////      default:
//////         return false;
//////   }
//////}

void CompilerLogic :: tweakPrimitiveClassFlags(ref_t classRef, ClassInfo& info)
{
   // if it is a primitive field
   if (info.fields.Count() == 1) {
      switch (classRef) {
         case V_DWORD:
         case V_INT32:
            info.header.flags |= elDebugDWORD;
            break;
         case V_PTR32:
            info.header.flags |= elDebugPTR;
            break;
         case V_INT64:
            info.header.flags |= elDebugQWORD;
            break;
         case V_REAL64:
            info.header.flags |= elDebugReal64;
            break;
//         case V_PTR:
//            info.header.flags |= (elDebugPTR | elWrapper);
//            info.fieldTypes.add(0, ClassInfo::FieldInfo(V_PTR, 0));
//            return info.size == 4;
         case V_SUBJECT:
            info.header.flags |= elDebugSubject | elSubject;
            break;
         case V_MESSAGE:
            info.header.flags |= (elDebugMessage | elMessage);
            break;
         case V_EXTMESSAGE:
            info.header.flags |= (elDebugMessage | elExtMessage);
            break;
         case V_SYMBOL:
            info.header.flags |= (elDebugReference | elSymbol);
            break;
         default:
            break;
      }
   }
}

ref_t CompilerLogic :: retrievePrimitiveReference(_ModuleScope&, ClassInfo& info)
{
   if (test(info.header.flags, elWrapper)) {
      ClassInfo::FieldInfo field = info.fieldTypes.get(0);
      if (isPrimitiveRef(field.value1))
         return field.value1;
   }

   return 0;
}

ref_t CompilerLogic :: definePrimitiveArray(_ModuleScope& scope, ref_t elementRef, bool structOne)
{
   ClassInfo info;
   if (!defineClassInfo(scope, info, elementRef, true))
      return 0;

   if (isEmbeddable(info) && structOne) {
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

//////bool CompilerLogic :: validateClassFlag(ClassInfo& info, int flag)
//////{
//////   if (test(flag, elDynamicRole) && info.fields.Count() != 0)
//////      return false;
//////
//////   return true;
//////}

void CompilerLogic :: validateClassDeclaration(_ModuleScope& scope, ClassInfo& info, bool& withAbstractMethods, bool& disptacherNotAllowed, bool& emptyStructure)
{
   if (!isAbstract(info)) {
      for (auto it = info.methodHints.start(); !it.Eof(); it++) {
         auto key = it.key();
         if (key.value2 == maHint && test(*it, tpAbstract)) {
            scope.printMessageInfo(infoAbstractMetod, key.value1);

            withAbstractMethods = true;
         }            
      }
   }

   // interface class cannot have a custom dispatcher method
   if (test(info.header.flags, elNoCustomDispatcher) && info.methods.exist(scope.dispatch_message, true))
      disptacherNotAllowed = true;

   // a structure class should contain fields
   if (test(info.header.flags, elStructureRole) && info.size == 0)
      emptyStructure = true;
}

bool CompilerLogic :: recognizeEmbeddableGet(_ModuleScope& scope, SNode root, /*ref_t extensionRef, */ref_t returningRef, ref_t& actionRef)
{
   if (returningRef != 0 && defineStructSize(scope, returningRef, 0) > 0) {
      root = root.findChild(lxNewFrame);

      if (root.existChild(lxReturning)) {
         SNode message = SyntaxTree::findPattern(root, 2,
            SNodePattern(lxExpression),
            SNodePattern(lxDirectCalling, lxSDirctCalling));

         // if it is eval&subject2:var[1] message
         if (getParamCount(message.argument) != 1)
            return false;

         // check if it is operation with $self
         SNode target = SyntaxTree::findPattern(root, 3,
            SNodePattern(lxExpression),
            SNodePattern(lxDirectCalling, lxSDirctCalling),
            SNodePattern(lxSelfLocal, lxLocal));
         if (target == lxNone) {
            target = SyntaxTree::findPattern(root, 4,
               SNodePattern(lxExpression),
               SNodePattern(lxDirectCalling, lxSDirctCalling),
               SNodePattern(lxExpression),
               SNodePattern(lxSelfLocal, lxLocal));
         }

//         if (target == lxLocal && target.argument == -1 && extensionRef != 0) {            
//            if (message.findChild(lxCallTarget).argument != extensionRef)
//               return false;
//         }
         /*else */if (target != lxSelfLocal || target.argument != 1)
            return false;

         // check if the argument is returned
         SNode arg = SyntaxTree::findPattern(root, 4,
            SNodePattern(lxExpression),
            SNodePattern(lxDirectCalling, lxSDirctCalling),
            SNodePattern(lxExpression),
            SNodePattern(lxLocalAddress));

         if (arg == lxNone) {
            arg = SyntaxTree::findPattern(root, 5,
               SNodePattern(lxExpression),
               SNodePattern(lxDirectCalling, lxSDirctCalling),
               SNodePattern(lxExpression),
               SNodePattern(lxExpression),
               SNodePattern(lxLocalAddress));
         }

         SNode ret = SyntaxTree::findPattern(root, 3,
            SNodePattern(lxReturning),
            SNodePattern(lxBoxing),
            SNodePattern(lxLocalAddress));

         if (arg != lxNone && ret != lxNone && arg.argument == ret.argument) {
            actionRef = getAction(message.argument);

            return true;
         }
      }
   }

   return false;
}

//bool CompilerLogic :: recognizeEmbeddableOp(_CompilerScope& scope, SNode root, ref_t extensionRef, ref_t returningRef, ref_t verb, ref_t& subject)
//{
//   if (returningRef != 0 && defineStructSize(scope, returningRef, 0) > 0) {
//      root = root.findChild(lxNewFrame);
//
//      if (root.existChild(lxReturning)) {
//         SNode message = SyntaxTree::findPattern(root, 2,
//            SNodePattern(lxExpression),
//            SNodePattern(lxDirectCalling, lxSDirctCalling));
//
//         // if it is read&subject&var[2] message
//         if (getParamCount(message.argument) != 2 || (verb != EVAL_MESSAGE_ID && getAction(message.argument) != (ref_t)verb))
//            return false;
//
//         // check if it is operation with $self
//         SNode target = SyntaxTree::findPattern(root, 3,
//            SNodePattern(lxExpression),
//            SNodePattern(lxDirectCalling, lxSDirctCalling),
//            SNodePattern(lxSelfLocal, lxLocal));
//
//         SNode indexArg;
//         if (target == lxNone) {
//            target = SyntaxTree::findPattern(root, 4,
//               SNodePattern(lxExpression),
//               SNodePattern(lxDirectCalling, lxSDirctCalling),
//               SNodePattern(lxExpression),
//               SNodePattern(lxSelfLocal, lxLocal));
//
//            indexArg = target.parentNode().nextNode(lxObjectMask);
//         }
//         else indexArg = target.nextNode(lxObjectMask);
//
//         if (target == lxLocal && target.argument == -1 && extensionRef != 0) {
//            if (message.findChild(lxCallTarget).argument != extensionRef)
//               return false;
//         }
//         else if (target != lxSelfLocal || target.argument != 1)
//            return false;
//
//         // check if the index is used
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
//         if (getParamCount(message.argument) != 3 || (verb != EVAL_MESSAGE_ID && getAction(message.argument) != verb))
//            return false;
//
//         // check if it is operation with $self
//         SNode target = SyntaxTree::findPattern(root, 3,
//            SNodePattern(lxExpression),
//            SNodePattern(lxDirectCalling, lxSDirctCalling),
//            SNodePattern(lxSelfLocal, lxLocal));
//
//         if (target == lxLocal && target.argument == -1 && extensionRef != 0) {
//            if (message.findChild(lxCallTarget).argument != extensionRef)
//               return false;
//         }
//         else if (target != lxSelfLocal || target.argument != 1)
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

bool CompilerLogic :: recognizeEmbeddableIdle(SNode methodNode, bool extensionOne)
{
   SNode object = SyntaxTree::findPattern(methodNode, 3,
      SNodePattern(lxNewFrame),
      SNodePattern(lxReturning),
      SNodePattern(extensionOne ? lxLocal : lxSelfLocal));

   return extensionOne ? (object == lxLocal && object.argument == -1) : (object == lxSelfLocal && object.argument == 1);
}

bool CompilerLogic :: recognizeEmbeddableMessageCall(SNode methodNode, ref_t& messageRef)
{
   SNode attr = methodNode.findChild(lxEmbeddableMssg);
   if (attr != lxNone) {
      messageRef = attr.argument;

      return true;
   }
   else return false;
}

bool CompilerLogic :: optimizeEmbeddableGet(_ModuleScope& scope, _Compiler& compiler, SNode node)
{
   SNode callNode = node.findSubNode(lxDirectCalling, lxSDirctCalling);
   SNode callTarget = callNode.findChild(lxCallTarget);

   ClassInfo info;
   if (!defineClassInfo(scope, info, callTarget.argument))
      return false;

   ref_t actionRef = info.methodHints.get(Attribute(callNode.argument, maEmbeddableGet));
   // if it is possible to replace get&subject operation with eval&subject2:local
   if (actionRef != 0) {
      compiler.injectEmbeddableGet(node, callNode, actionRef);

      return true;
   }
   else return false;
}

bool CompilerLogic :: optimizeEmbeddableOp(_ModuleScope& scope, _Compiler& compiler, SNode node)
{
   SNode callNode = node.findSubNode(lxDirectCalling, lxSDirctCalling);
   SNode callTarget = callNode.findChild(lxCallTarget);

   ClassInfo info;
   if(!defineClassInfo(scope, info, callTarget.argument))
      return false;

   for (int i = 0; i < EMBEDDABLEOP_MAX; i++) {
      EmbeddableOp op = embeddableOps[i];
      ref_t subject = info.methodHints.get(Attribute(callNode.argument, op.attribute));

      //ref_t initConstructor = encodeMessage(INIT_MESSAGE_ID, 0) | SPECIAL_MESSAGE;

      // if it is possible to replace get&subject operation with eval&subject2:local
      if (subject != 0) {
         compiler.injectEmbeddableOp(scope, node, callNode, subject, op.paramCount/*, op.verb*/);

         return true;
      }
   }

   return false;
}

bool CompilerLogic :: validateBoxing(_ModuleScope& scope, _Compiler& compiler, SNode& node, ref_t targetRef, ref_t sourceRef, bool unboxingExpected, bool dynamicRequired)
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
   //else if (isPrimitiveRef(sourceRef) && (isCompatible(scope, targetRef, resolvePrimitiveReference(scope, sourceRef)) || sourceRef == V_INT32)) {
   //   //HOTFIX : allowing numeric constant direct boxing
   //}
   else if (node.existChild(lxBoxableAttr)) {
      // HOTFIX : if the object was explicitly boxed
   }
   else return false;

   if (!dynamicRequired) {
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
   }

   return true;
}

////void CompilerLogic :: injectVariableAssigning(SyntaxWriter& writer, _CompilerScope& scope, _Compiler& compiler, ref_t& targetRef, ref_t& type, int& operand, bool paramMode)
////{
////   ClassInfo info;
////   defineClassInfo(scope, info, targetRef);
////
////   operand = defineStructSize(info, false);
////   
////   if (paramMode) {
////      if (operand == 0) {
////         //HOTFIX : allowing to assign a reference variable
////         // replace the parameter with the field expression
////         compiler.injectFieldExpression(writer);
////      }
////
////      type = info.fieldTypes.get(0).value2;
////      targetRef = info.fieldTypes.get(0).value1;
////   }
////}

bool CompilerLogic :: optimizeEmbeddable(SNode node, _ModuleScope& scope)
{
   // check if it is a virtual call
   if (node == lxDirectCalling && getParamCount(node.argument) == 0) {
      SNode callTarget = node.findChild(lxCallTarget);

      ClassInfo info;
      if (defineClassInfo(scope, info, callTarget.argument) && info.methodHints.get(Attribute(node.argument, maEmbeddableIdle)) == -1) {
         // if it is an idle call, remove it
         node = lxExpression;

         return true;
      }
   }

   return false;
}

void CompilerLogic :: optimizeBranchingOp(_ModuleScope&, SNode node)
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
            if (intOpNode.argument == LESS_OPERATOR_ID) {
               intOpNode.argument = GREATER_OPERATOR_ID;
            }
            else if (intOpNode.argument == GREATER_OPERATOR_ID) {
               intOpNode.argument = LESS_OPERATOR_ID;
            }
         }

         if (intOpNode.argument == EQUAL_OPERATOR_ID) {
            if (trueNode.argument == ifNode.argument) {
               ifNode.set(lxIfN, arg);
            }
            else if (trueNode.argument != ifNode.argument) {
               ifNode.set(lxIfNotN, arg);
            }
            else return;
         }
         else if (intOpNode.argument == LESS_OPERATOR_ID) {
            if (trueNode.argument != ifNode.argument) {
               ifNode.set(lxLessN, arg);
            }
            else if (trueNode.argument == ifNode.argument) {
               ifNode.set(lxNotLessN, arg);
            }
            else return;
         }
         else if (intOpNode.argument == GREATER_OPERATOR_ID) {
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

////inline void readFirstSignature(ident_t signature, size_t& start, size_t& end, IdentifierString& temp)
////{
////   start = signature.find('$') + 1;
////   end = signature.find(start, '$', getlength(signature));
////   temp.copy(signature.c_str() + start, end - start);
////}
////
////inline void readNextSignature(ident_t signature, size_t& start, size_t& end, IdentifierString& temp)
////{
////   end = signature.find(start, '$', getlength(signature));
////   temp.copy(signature.c_str() + start, end - start);
////}

ref_t CompilerLogic :: resolveMultimethod(_ModuleScope& scope, ref_t multiMessage, ref_t targetRef, ref_t implicitSignatureRef, int& stackSafeAttr)
{
   if (!targetRef)
      return 0;

   ClassInfo info;
   if (defineClassInfo(scope, info, targetRef)) {
      if (isMethodInternal(info, multiMessage)) {
         // recognize the internal message
         ref_t signRef = 0;
         IdentifierString internalName(scope.module->Name(), "$$");
         internalName.append(scope.module->resolveAction(getAction(multiMessage), signRef));

         ref_t internalRef = scope.module->mapAction(internalName.c_str(), signRef, false);

         multiMessage = overwriteAction(multiMessage, internalRef);
         if (!implicitSignatureRef)
            // if no signature provided - return the general internal message
            return multiMessage;
      }

      if (!implicitSignatureRef)
         return 0;

      ref_t signatures[ARG_COUNT];
      scope.module->resolveSignature(implicitSignatureRef, signatures);

      ref_t listRef = info.methodHints.get(Attribute(multiMessage, maOverloadlist));
      if (listRef == 0 && isMethodPrivate(info, multiMessage))
         listRef = info.methodHints.get(Attribute(multiMessage | STATIC_MESSAGE, maOverloadlist));

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
               if (isSignatureCompatible(scope, argSign, signatures)) {
                  if (isEmbeddable(info))
                     stackSafeAttr |= 1;

                  setSignatureStacksafe(scope, argSign, stackSafeAttr);

                  return argMessage;
               }                  
            }
            else {
               if (isSignatureCompatible(scope, argModule, argSign, signatures)) {
                  if (isEmbeddable(info))
                     stackSafeAttr |= 1;

                  setSignatureStacksafe(scope, argModule, argSign, stackSafeAttr);

                  return importMessage(argModule, argMessage, scope.module);
               }                  
            }

            position -= 8;
         }
      }
   }

   return 0;
}

inline size_t readSignatureMember(ident_t signature, size_t index)
{
   int level = 0;
   size_t len = getlength(signature);
   for (size_t i = index; i < len; i++) {
      if (signature[i] == '&') {
         if (level == 0) {
            return i;
         }
         else level--;
      }
      else if (signature[i] == '#') {
         String<char, 5> tmp;
         size_t numEnd = signature.find(i, '&', NOTFOUND_POS);
         tmp.copy(signature.c_str() + i + 1, numEnd - i - 1);
         level += ident_t(tmp).toInt();
      }
   }

   return len;
}

inline void decodeClassName(IdentifierString& signature)
{
   ident_t ident = signature.ident();

   if (ident.startsWith(TEMPLATE_PREFIX_NS_ENCODED)) {
      // if it is encodeded weak reference - decode only the prefix
      signature[0] = '\'';
      signature[strlen(TEMPLATE_PREFIX_NS_ENCODED) - 1] = '\'';
   }
   else if (ident.startsWith(TEMPLATE_PREFIX_NS)) {
      // if it is weak reference - do nothing
   }
   else signature.replaceAll('@', '\'', 0);
}

ref_t CompilerLogic :: resolveExtensionTemplate(_ModuleScope& scope, _Compiler& compiler, ident_t pattern, ref_t implicitSignatureRef, ident_t ns)
{
   size_t argumentLen = 0;
   ref_t parameters[ARG_COUNT] = { 0 };
   ref_t signatures[ARG_COUNT];
   scope.module->resolveSignature(implicitSignatureRef, signatures);

   // matching pattern with the provided signature
   size_t i = pattern.find('.') + 2;

   IdentifierString templateName(pattern, i - 2);
   ref_t templateRef = scope.mapFullReference(templateName.ident(), true);

   size_t len = getlength(pattern);
   bool matched = true;
   size_t signIndex = 0;
   while (matched && i < len) {
      if (pattern[i] == '{') {
         size_t end = pattern.find(i, '}', 0);

         String<char, 5> tmp;
         tmp.copy(pattern + i + 1, end - i - 1);

         int index = ident_t(tmp).toInt();

         parameters[index - 1] = signatures[signIndex];
         if (argumentLen < index)
            argumentLen = index;

         i = end + 2;
      }
      else {
         size_t end = pattern.find(i, '/', getlength(pattern));
         IdentifierString argType;
         argType.copy(pattern + i, end - i);

         if (argType.ident().find('{') != NOTFOUND_POS) {
            ref_t argRef = signatures[signIndex];
            // bad luck : if it is a template based argument
            ident_t signType;
            while (argRef) {
               // try to find the template based signature argument
               signType = scope.module->resolveReference(argRef);
               if (!isTemplateWeakReference(signType)) {
                  ClassInfo info;
                  defineClassInfo(scope, info, argRef, true);
                  argRef = info.header.parentRef;
               }
               else break;
            }

            if (argRef) {
               size_t argLen = getlength(argType);
               size_t start = 0;
               size_t argIndex = argType.ident().find('{');
               while (argIndex < argLen && matched) {
                  if (argType.ident().compare(signType, start, argIndex - start)) {
                     size_t paramEnd = argType.ident().find(argIndex, '}', 0);

                     String<char, 5> tmp;
                     tmp.copy(argType.c_str() + argIndex + 1, paramEnd - argIndex - 1);

                     IdentifierString templateArg;
                     size_t nextArg = readSignatureMember(signType, argIndex - start);
                     templateArg.copy(signType + argIndex - start, nextArg - argIndex + start);
                     decodeClassName(templateArg);                     

                     signType = signType + nextArg + 1;

                     size_t index = ident_t(tmp).toInt();
                     ref_t templateArgRef = scope.mapFullReference(templateArg);
                     if (!parameters[index - 1]) {
                        parameters[index - 1] = templateArgRef;
                     }
                     else if (parameters[index - 1] != templateArgRef) {
                        matched = false;
                        break;
                     }
                     
                     if (argumentLen < index)
                        argumentLen = index;

                     start = paramEnd + 2;
                     argIndex = argType.ident().find(start, '{', argLen);
                  }
                  else matched = false;
               }

               if (matched && start < argLen) {
                  // validate the rest part
                  matched = argType.ident().compare(signType, start, argIndex - start);
               }
            }
            else matched = false;
         }
         else {
            ref_t argRef = scope.mapFullReference(argType.ident(), true);
            matched = isCompatible(scope, argRef, signatures[signIndex]);
         }

         i = end + 1;
      }

      signIndex++;
   }

   if (matched) {
      return compiler.generateExtensionTemplate(scope, templateRef, argumentLen, parameters, ns);
   }
   
   return 0;
}

bool CompilerLogic :: validateMessage(_ModuleScope& scope, ref_t message, bool isClassClass)
{
   bool dispatchOne = message == scope.dispatch_message;

   if (isClassClass && dispatchOne) {
      return false;
   }
   //else if (!isClassClass && dispatchOne && getParamCount(message) != 0) {
   //   return false;
   //}
   else return true;
}
