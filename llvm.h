#ifndef INCLUDE_llvm_h
#define INCLUDE_llvm_h

#ifdef USE_STANDARD_HEADERS

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>

#else

/* <llvm-c/Core.h> */
#define LLVMIntEQ 32
#define LLVMIntNE 33
#define LLVMIntSGT 38
#define LLVMIntSGE 39
#define LLVMIntSLT 40
#define LLVMIntSLE 41

typedef struct LLVMOpaqueContext *LLVMContextRef;
typedef struct LLVMOpaqueBuilder *LLVMBuilderRef;
typedef struct LLVMOpaqueModule *LLVMModuleRef;
typedef struct LLVMOpaqueBasicBlock *LLVMBasicBlockRef;
typedef struct LLVMOpaqueValue *LLVMValueRef;
typedef struct LLVMOpaqueType *LLVMTypeRef;

LLVMModuleRef LLVMModuleCreateWithName(const char *module_id);
LLVMContextRef LLVMGetModuleContext(LLVMModuleRef module);
LLVMValueRef LLVMAddGlobal(LLVMModuleRef module, LLVMTypeRef type,
                           const char *name);
LLVMValueRef LLVMGetNamedGlobal(LLVMModuleRef module, const char *name);
LLVMValueRef LLVMAddFunction(LLVMModuleRef module, const char *name,
                             LLVMTypeRef func_type);
LLVMValueRef LLVMGetNamedFunction(LLVMModuleRef module, const char *name);
void LLVMDisposeModule(LLVMModuleRef module);

LLVMBuilderRef LLVMCreateBuilder(void);
void LLVMPositionBuilderAtEnd(LLVMBuilderRef b, LLVMBasicBlockRef bb);
LLVMBasicBlockRef LLVMGetInsertBlock(LLVMBuilderRef b);
void LLVMDisposeBuilder(LLVMBuilderRef b);
LLVMValueRef LLVMBuildGlobalStringPtr(LLVMBuilderRef b, const char *str,
                                      const char *name);
LLVMValueRef LLVMConstInt(LLVMTypeRef type, unsigned long n, int sign_extend);
LLVMValueRef LLVMConstNull(LLVMTypeRef type);
LLVMValueRef LLVMBuildICmp(LLVMBuilderRef b, int op, LLVMValueRef left,
                           LLVMValueRef right, const char *name);
LLVMValueRef LLVMBuildIsNull(LLVMBuilderRef b, LLVMValueRef val,
                             const char *name);
LLVMValueRef LLVMBuildIsNotNull(LLVMBuilderRef b, LLVMValueRef val,
                                const char *name);
LLVMValueRef LLVMBuildAdd(LLVMBuilderRef b, LLVMValueRef left,
                          LLVMValueRef right, const char *name);
LLVMValueRef LLVMBuildSub(LLVMBuilderRef b, LLVMValueRef left,
                          LLVMValueRef right, const char *name);
LLVMValueRef LLVMBuildMul(LLVMBuilderRef b, LLVMValueRef left,
                          LLVMValueRef right, const char *name);
LLVMValueRef LLVMBuildSDiv(LLVMBuilderRef b, LLVMValueRef left,
                           LLVMValueRef right, const char *name);
LLVMValueRef LLVMBuildSRem(LLVMBuilderRef b, LLVMValueRef left,
                           LLVMValueRef right, const char *name);
LLVMValueRef LLVMBuildPtrDiff(LLVMBuilderRef b, LLVMValueRef left,
                              LLVMValueRef right, const char *name);
LLVMValueRef LLVMBuildAnd(LLVMBuilderRef b, LLVMValueRef left,
                          LLVMValueRef right, const char *name);
LLVMValueRef LLVMBuildOr(LLVMBuilderRef b, LLVMValueRef left,
                         LLVMValueRef right, const char *name);
LLVMValueRef LLVMBuildXor(LLVMBuilderRef b, LLVMValueRef left,
                          LLVMValueRef right, const char *name);
LLVMValueRef LLVMBuildNeg(LLVMBuilderRef b, LLVMValueRef val, const char *name);
LLVMValueRef LLVMBuildTrunc(LLVMBuilderRef b, LLVMValueRef val,
                            LLVMTypeRef type, const char *name);
LLVMValueRef LLVMBuildSExt(LLVMBuilderRef b, LLVMValueRef val, LLVMTypeRef type,
                           const char *name);
LLVMValueRef LLVMBuildZExt(LLVMBuilderRef b, LLVMValueRef val, LLVMTypeRef type,
                           const char *name);
LLVMValueRef LLVMBuildIntToPtr(LLVMBuilderRef b, LLVMValueRef val,
                               LLVMTypeRef type, const char *name);
LLVMValueRef LLVMBuildPtrToInt(LLVMBuilderRef b, LLVMValueRef val,
                               LLVMTypeRef type, const char *name);
LLVMValueRef LLVMBuildPointerCast(LLVMBuilderRef b, LLVMValueRef val,
                                  LLVMTypeRef type, const char *name);
LLVMValueRef LLVMBuildLoad(LLVMBuilderRef b, LLVMValueRef ptr,
                           const char *name);
LLVMValueRef LLVMBuildStore(LLVMBuilderRef b, LLVMValueRef val,
                            LLVMValueRef ptr);
LLVMValueRef LLVMBuildInBoundsGEP(LLVMBuilderRef b, LLVMValueRef ptr,
                                  LLVMValueRef *indices,
                                  unsigned int num_indices, const char *name);
LLVMValueRef LLVMBuildInBoundsGEP(LLVMBuilderRef b, LLVMValueRef ptr,
                                  LLVMValueRef *indices,
                                  unsigned int num_indices, const char *name);
LLVMValueRef LLVMBuildStructGEP(LLVMBuilderRef b, LLVMValueRef ptr,
                                unsigned int index, const char *name);
LLVMValueRef LLVMBuildExtractValue(LLVMBuilderRef b, LLVMValueRef val,
                                   unsigned int index, const char *name);
LLVMValueRef LLVMBuildCall(LLVMBuilderRef b, LLVMValueRef func,
                           LLVMValueRef *args, unsigned int num_args,
                           const char *name);
LLVMValueRef LLVMBuildAlloca(LLVMBuilderRef b, LLVMTypeRef type,
                             const char *name);

LLVMValueRef LLVMBuildRetVoid(LLVMBuilderRef b);
LLVMValueRef LLVMBuildRet(LLVMBuilderRef b, LLVMValueRef val);
LLVMValueRef LLVMBuildBr(LLVMBuilderRef b, LLVMBasicBlockRef dest);
LLVMValueRef LLVMBuildCondBr(LLVMBuilderRef b, LLVMValueRef if_,
                             LLVMBasicBlockRef then, LLVMBasicBlockRef else_);
LLVMValueRef LLVMBuildSwitch(LLVMBuilderRef b, LLVMValueRef value,
                             LLVMBasicBlockRef default_,
                             unsigned int num_cases);
void LLVMAddCase(LLVMValueRef switch_, LLVMValueRef case_value,
                 LLVMBasicBlockRef dest);

LLVMValueRef LLVMBuildPhi(LLVMBuilderRef b, LLVMTypeRef type, const char *name);
void LLVMAddIncoming(LLVMValueRef phi, LLVMValueRef *incoming_values,
                     LLVMBasicBlockRef *incoming_blocks, unsigned int count);

LLVMValueRef LLVMGetBasicBlockParent(LLVMBasicBlockRef bb);

LLVMTypeRef LLVMTypeOf(LLVMValueRef val);
void LLVMSetInitializer(LLVMValueRef global_var, LLVMValueRef constant_val);

LLVMValueRef LLVMGetParam(LLVMValueRef func, unsigned int index);
LLVMBasicBlockRef LLVMAppendBasicBlock(LLVMValueRef func, const char *name);

LLVMTypeRef LLVMVoidType(void);
LLVMTypeRef LLVMInt1Type(void);
LLVMTypeRef LLVMInt8Type(void);
LLVMTypeRef LLVMInt32Type(void);
LLVMTypeRef LLVMPointerType(LLVMTypeRef element_type,
                            unsigned int address_space);
LLVMTypeRef LLVMArrayType(LLVMTypeRef element_type, unsigned int length);
LLVMTypeRef LLVMStructCreateNamed(LLVMContextRef context, const char *name);
LLVMTypeRef LLVMFunctionType(LLVMTypeRef return_type, LLVMTypeRef *param_types,
                             unsigned int param_count, int is_var_arg);
LLVMTypeRef LLVMGetElementType(LLVMTypeRef type);
LLVMTypeRef LLVMGetReturnType(LLVMTypeRef func_type);
unsigned int LLVMCountParamTypes(LLVMTypeRef func_type);
void LLVMGetParamTypes(LLVMTypeRef func_type, LLVMTypeRef *dest);

void LLVMStructSetBody(LLVMTypeRef struct_type, LLVMTypeRef *element_types,
                       unsigned int element_count, int packed);

char *LLVMPrintModuleToString(LLVMModuleRef module);
char *LLVMPrintTypeToString(LLVMTypeRef type);

void LLVMDisposeMessage(char *message);

/* <llvm-c/Analysis.h> */
#define LLVMReturnStatusAction 2

int LLVMVerifyModule(LLVMModuleRef module, int action, char **message);

#endif

#endif
