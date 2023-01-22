/*
* Copyright (C) 2017-2018 Huawei Technologies Co., Ltd.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include <iostream>
#include <climits>
#include <cmath>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include "cpphelper.h"
#include "ScopedUtfChars.h"
#include "native/mpl_native_stack.h"
#include "thread.h"
#include "exception/stack_unwinder.h"
#include "utils/name_validator.h"
#include "runtime.h"
#include "class_linker.h"
#include "monitor.h"
#include "threadstate.h"
#include "base/systrace.h"
#include "well_known_classes.h"
#include "classfile_constants.h"
#include "interp_support.h"
#include "methodmeta_inline.h"
#include "mrt_linker_api.h"

using namespace std;
using namespace maple;

static uint16_t ByteSwap(uint16_t x) {
  return (x << 8) | (x >> 8);
}

static uint32_t ByteSwap(uint32_t x) {
  auto lower = static_cast<uint16_t>(x);
  auto high = static_cast<uint16_t>(x >> 16);
  return (static_cast<uint32_t>(ByteSwap(lower)) << 16) | (static_cast<uint32_t>(ByteSwap(high)));
}

static uint64_t ByteSwap(uint64_t x) {
  auto lower = static_cast<uint32_t>(x);
  auto high = static_cast<uint32_t>(x >> 32);
  return (static_cast<uint64_t>(ByteSwap(lower)) << 32) | (static_cast<uint64_t>(ByteSwap(high)));
}

template<typename T>
static void SwapMemory(jbyte *src, jbyte *dst, size_t byteCount){
  int typeSize = sizeof(T);
  bool copyDirectionIsLeft = false;
  if (dst > src && dst < src) {
    copyDirectionIsLeft = true;
  }
  jbyte *tmpSrc = src;
  jbyte *tmpDst = dst;
  if (copyDirectionIsLeft) {
    tmpSrc = src + byteCount - typeSize;
    tmpDst = dst + byteCount - typeSize;
  }
  size_t elemCount;
  elemCount = byteCount / typeSize;
  for (size_t i = 0; i < elemCount; i++) {
    T tmp;
    tmp = *reinterpret_cast<T*>(tmpSrc);
    tmp = ByteSwap(tmp);
    *reinterpret_cast<T*>(tmpDst) = tmp;
    if (copyDirectionIsLeft) {
      tmpSrc -= typeSize;
      tmpDst -= typeSize;
    } else {
      tmpSrc += typeSize;
      tmpDst += typeSize;
    }
  }
}

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define JVM_INTERFACE_VERSION 4

JNIEXPORT jint JVM_GetInterfaceVersion(void) {
  return JVM_INTERFACE_VERSION;
}

/*
 * java.lang.Object
 */
JNIEXPORT jint JVM_IHashCode(JNIEnv *env ATTR_UNUSED, jobject obj) {
  jint hashcode = maple::ObjectBase::IdentityHashCode(reinterpret_cast<address_t>(obj));
  return hashcode;
}

JNIEXPORT void JVM_MonitorWait(JNIEnv *env ATTR_UNUSED, jobject obj, jlong ms) {
  maple::ObjectBase::Wait(reinterpret_cast<address_t>(obj), ms, 0);
}

JNIEXPORT void JVM_MonitorNotify(JNIEnv *env ATTR_UNUSED, jobject obj) {
  maple::ObjectBase::Notify(reinterpret_cast<address_t>(obj));
}

JNIEXPORT void JVM_MonitorNotifyAll(JNIEnv *env ATTR_UNUSED, jobject obj) {
  maple::ObjectBase::NotifyAll(reinterpret_cast<address_t>(obj));
}

JNIEXPORT jobject JVM_Clone(JNIEnv *env, jobject obj) {
  maplert::ScopedObjectAccess soa;
  return maplert::MRT_JNI_AddLocalReference(env, MRT_CloneJavaObject(obj));
}

/*
 * java.lang.String
 */
JNIEXPORT jstring JVM_InternString(JNIEnv *env ATTR_UNUSED, jstring str) {
  UNIMPLEMENTED(FATAL) << "JVM_InternString should not be invoked";
  // only used in the JNI native of java/lang/String which
  // invokes Java_java_lang_String_intern__  instead in maplert
  return str;
}

/*
 * java.lang.System
 */
JNIEXPORT jlong JVM_CurrentTimeMillis(JNIEnv *env ATTR_UNUSED, jclass ignored ATTR_UNUSED) {
  struct timeval tv;
  (void)gettimeofday(&tv, (struct timezone*)nullptr);
  jlong when = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
  return when;
}

JNIEXPORT jlong JVM_NanoTime(JNIEnv *env ATTR_UNUSED, jclass ignored ATTR_UNUSED) {
  return timeutils::NanoSeconds();
}

extern "C"  MRT_EXPORT void Native_java_lang_System_arraycopy__Ljava_lang_Object_2ILjava_lang_Object_2II (
  jobject javaSrc,  // source array
  jint srcPos,      // start idx
  jobject javaDst,  // dst array
  jint dstPos,      // start idx
  jint length);

JNIEXPORT void JVM_ArrayCopy(JNIEnv *env ATTR_UNUSED, jclass ignored ATTR_UNUSED, jobject src, jint srcPos,
                             jobject dst, jint dstPos, jint length) {
  maplert::ScopedObjectAccess soa;
  Native_java_lang_System_arraycopy__Ljava_lang_Object_2ILjava_lang_Object_2II(src, srcPos, dst, dstPos, length);
}

JNIEXPORT jobject JVM_InitProperties(JNIEnv *env, jobject properties) {
  maplert::ScopedObjectAccess soa;
  const char *javaHome = "java.home";
  char *path = getenv("MAPLE_JAVA_HOME");
  if (path != nullptr) {
    jstring key = env->NewStringUTF(javaHome);
    jstring value = env->NewStringUTF(path);
    jmethodID putID = env->GetMethodID(env->GetObjectClass(properties), "put",
        "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    jobject result = env->CallObjectMethod(properties, putID, key, value);
    (void)result;
  }
  const char *javaLibrary = "java.library.path";
  char *libPath = getenv("LD_LIBRARY_PATH");
  if (libPath != nullptr) {
    jstring key = env->NewStringUTF(javaLibrary);
    jstring value = env->NewStringUTF(libPath);
    jmethodID putID = env->GetMethodID(env->GetObjectClass(properties), "put",
        "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    jobject resultLib = env->CallObjectMethod(properties, putID, key, value);
    (void)resultLib;
  }
  return properties;
}

/*
 * java.io.File
 */
JNIEXPORT void JVM_OnExit(void (*func)(void) ATTR_UNUSED) {
  // not exit on openJDK
  UNIMPLEMENTED(FATAL) << "JVM_OnExit won't be called forever!";
}

/*
 * java.nio.Bits
 */
JNIEXPORT void JVM_CopySwapMemory(JNIEnv *env ATTR_UNUSED, jobject srcObj, jlong srcOffset,
    jobject dstObj, jlong dstOffset, jlong size, jlong elemSize) {
  jbyte *src = nullptr;
  jbyte *dst = nullptr;
  src = reinterpret_cast<jbyte*>(reinterpret_cast<intptr_t>(srcObj) + srcOffset);
  dst = reinterpret_cast<jbyte*>(reinterpret_cast<intptr_t>(dstObj) + dstOffset);

  if (src == nullptr || dst == nullptr) {
    LOG(FATAL) << "JVM_CopySwapMemory error: address is null";
    return;
  }
  size_t elemSizeLocal = static_cast<size_t>(elemSize);
  if (elemSizeLocal != 2 && elemSizeLocal != 4 && elemSizeLocal != 8) {
    LOG(FATAL) << "JVM_CopySwapMemory error: incorrect element size " << elemSizeLocal;
    return;
  }
  size_t byteCount = static_cast<size_t>(size);
  if (((byteCount + elemSizeLocal - 1) & ~(elemSizeLocal - 1)) != byteCount) {
    LOG(FATAL) << "JVM_CopySwapMemory error: byte_count=" << byteCount << " is not multiple of element_size=" << elemSizeLocal;
    return;
  }

  switch (elemSizeLocal) {
    case 2:
      SwapMemory<uint16_t>(src, dst, byteCount);
      break;
    case 4:
      SwapMemory<uint32_t>(src, dst, byteCount);
      break;
    case 8:
      SwapMemory<uint64_t>(src, dst, byteCount);
      break;
    default:
      LOG(FATAL) << "JVM_CopySwapMemory error: Invalid element_size=" << elemSizeLocal;
  }
}

/*
 * java.lang.Runtime
 */
JNIEXPORT void JVM_Exit(jint code) {
  exit(code);
}

JNIEXPORT void JVM_Halt(jint code) {
  _exit(code);
}

JNIEXPORT void JVM_GC(void) {
  MRT_TriggerGC(maplert::kGCReasonUser);
}

// moved from Runtime.c to here:
JNIEXPORT void JNICALL
Java_java_lang_Runtime_runFinalization0(JNIEnv *unusedEnv ATTR_UNUSED, jobject unusedThis ATTR_UNUSED) {
  MRT_RunFinalization();
}

/* Returns the number of real-time milliseconds that have elapsed since the
 * least-recently-inspected heap object was last inspected by the garbage
 * collector.
 *
 * For simple stop-the-world collectors this value is just the time
 * since the most recent collection.  For generational collectors it is the
 * time since the oldest generation was most recently collected.  Other
 * collectors are free to return a pessimistic estimate of the elapsed time, or
 * simply the time since the last full collection was performed.
 *
 * Note that in the presence of reference objects, a given object that is no
 * longer strongly reachable may have to be inspected multiple times before it
 * can be reclaimed.
 */
JNIEXPORT jlong JVM_MaxObjectInspectionAge(void) {
  // no need to implement, not extis in openJDK
  UNIMPLEMENTED(FATAL) << "JVM_MaxObjectInspectionAge won't be called forever!";
  return 0L;
}

JNIEXPORT void JVM_TraceInstructions(jboolean on ATTR_UNUSED) {
  // not be called in openJDK
  UNIMPLEMENTED(FATAL) << "JVM_TraceInstructions won't be called forever!";
}

JNIEXPORT void JVM_TraceMethodCalls(jboolean on ATTR_UNUSED) {
  // not be called in openJDK
  UNIMPLEMENTED(FATAL) << "JVM_TraceMethodCalls won't be called forever!";
}

JNIEXPORT jlong JVM_TotalMemory(void) {
  return MRT_TotalMemory();
}

JNIEXPORT jlong JVM_FreeMemory(void) {
  return MRT_FreeMemory();
}

JNIEXPORT jlong JVM_MaxMemory(void) {
  return MRT_MaxMemory();
}

JNIEXPORT jint JVM_ActiveProcessorCount(void) {
  jint count = 0;
  cpu_set_t cs;
  CPU_ZERO(&cs);
  sched_getaffinity(getpid(), sizeof(cs), &cs);
  for (unsigned i = 0; i < sizeof(cs); i++) {
    if (CPU_ISSET(i, &cs)) {
      count++;
    }
  }
  return count;

}

JNIEXPORT void * JVM_LoadLibrary(const char *name) {
  if (name == nullptr) {
    return nullptr;
  }
  std::string error_msg;
  Thread* self = Thread::Current();
  JNIEnv* env = self->GetJniEnv();
  maple::JavaVMExt* vm = maple::Runtime::Current()->GetJavaVM();
  void* handle = vm->LoadNativeLibrary(env, name, nullptr, nullptr, &error_msg);
  if (handle == nullptr) {
    LOG(ERROR) << "JVM_LoadLibrary add "<< name <<" error: " << error_msg;
  }
  return handle;
}

JNIEXPORT void JVM_UnloadLibrary(void * handle) {
  if (handle != nullptr) {
    dlclose(handle);
  }
}

JNIEXPORT void * JVM_FindLibraryEntry(void *handle, const char *name) {
  return dlsym(handle, name);
}

JNIEXPORT jboolean JVM_IsSupportedJNIVersion(jint version ATTR_UNUSED) {
  return JNI_TRUE;
}

/*
 * java.lang.Float and java.lang.Double
 */
JNIEXPORT jboolean JVM_IsNaN(jdouble d) {
  return isnan(d);
}

/*
 * java.lang.Throwable
 */
JNIEXPORT void JVM_FillInStackTrace(JNIEnv *env, jobject throwable) {
  vector<uint64_t> trace;
  MapleStack::FastRecordCurrentStackPCs(trace);
  MRT_CheckException(!trace.empty(), "error: JavaCallStack is empty");

  jobject backtrace = reinterpret_cast<jobject>(StackFramePCsToLongArray(env, trace));
  // save to throwable.backtrace
  env->SetObjectField(throwable, WellKnownClasses::java_lang_Throwable_backtrace, backtrace);
}

JNIEXPORT jint JVM_GetStackTraceDepth(JNIEnv *env, jobject throwable) {
  jfieldID stackTraceId = WellKnownClasses::java_lang_Throwable_stackTrace;
  ScopedLocalRef<jobjectArray> stackTrace(env, (jobjectArray)env->GetObjectField(throwable, stackTraceId));
  jobjectArray javaStackTrace = stackTrace.get();
  jint traceCount = env->GetArrayLength(reinterpret_cast<jarray>(javaStackTrace));

  return traceCount;
}

// added native interface for getOurStackTrace
// returns a jobjectArray of StackTraceElement
JNIEXPORT jobjectArray Java_java_lang_Throwable_getStackTrace0(JNIEnv *env, jobject throwable) {
  jobject backtrace = env->GetObjectField(throwable, WellKnownClasses::java_lang_Throwable_backtrace);
  if (backtrace == nullptr) {
    return nullptr;
  }

  vector<maple_native_stack_item> stacktrace;
  JavaStackFramePCsToInternalStackTrace(env, backtrace, stacktrace);
  jobjectArray stackTrace = InternalStackTraceToStackTraceElementArray(env, stacktrace);
  // set throwable.backtrace to nullptr
  env->SetObjectField(throwable, WellKnownClasses::java_lang_Throwable_backtrace, nullptr);
  return stackTrace;
}

JNIEXPORT jobject JVM_GetStackTraceElement(JNIEnv *env ATTR_UNUSED,
                                           jobject throwable ATTR_UNUSED, jint index ATTR_UNUSED) {
  // LOG(FATAL) << "JVM_GetStackTraceElement is not implemented";
  // Not used any longer
  return nullptr;
}

/*
 * java.lang.Compiler
 */
JNIEXPORT void JVM_InitializeCompiler (JNIEnv *env ATTR_UNUSED, jclass compCls ATTR_UNUSED) {
  LOG(FATAL) << "JVM_InitializeCompiler is not implemented";
}

JNIEXPORT jboolean JVM_IsSilentCompiler(JNIEnv *env ATTR_UNUSED, jclass compCls ATTR_UNUSED) {
  LOG(FATAL) << "JVM_IsSilentCompiler is not implemented";
  return JNI_FALSE;
}

JNIEXPORT jboolean JVM_CompileClass(JNIEnv *env ATTR_UNUSED, jclass compCls ATTR_UNUSED,
                                    jclass cls ATTR_UNUSED) {
  LOG(FATAL) << "JVM_CompileClass is not implemented";
  return JNI_FALSE;
}

JNIEXPORT jboolean JVM_CompileClasses(JNIEnv *env ATTR_UNUSED, jclass cls ATTR_UNUSED,
                                      jstring jname ATTR_UNUSED) {
  LOG(FATAL) << "JVM_CompileClasses is not implemented";
  return JNI_FALSE;
}

JNIEXPORT jobject JVM_CompilerCommand(JNIEnv *env ATTR_UNUSED, jclass compCls ATTR_UNUSED,
                                      jobject arg ATTR_UNUSED) {
  LOG(FATAL) << "JVM_CompilerCommand is not implemented";
  return nullptr;
}

JNIEXPORT void JVM_EnableCompiler(JNIEnv *env ATTR_UNUSED, jclass compCls ATTR_UNUSED) {
  LOG(FATAL) << "JVM_EnableCompiler is not implemented";
}

JNIEXPORT void JVM_DisableCompiler(JNIEnv *env ATTR_UNUSED, jclass compCls ATTR_UNUSED) {
  LOG(FATAL) << "JVM_DisableCompiler is not implemented";
}

/*
 * java.lang.Thread
 */
JNIEXPORT void JVM_StartThread(JNIEnv *env, jobject javaThread, jlong stackSize, jboolean daemon) {
  VLOG(thread) << "Java_Thread_nativeCreate call" << "stackSize: " << stackSize << "daemon: " << daemon;
  Thread::CreateNativeThread(env, javaThread, stackSize, daemon == JNI_TRUE);
}

JNIEXPORT void JVM_StopThread(JNIEnv *env ATTR_UNUSED, jobject thread ATTR_UNUSED,
                              jobject exception ATTR_UNUSED) {
  LOG(FATAL) << "JVM_StopThread is not implemented";
}

JNIEXPORT jboolean JVM_IsThreadAlive(JNIEnv *env ATTR_UNUSED, jobject thread ATTR_UNUSED) {
  LOG(FATAL) << "JVM_IsThreadAlive is not implemented";
  return JNI_FALSE;
}

JNIEXPORT void JVM_SuspendThread(JNIEnv *env ATTR_UNUSED, jobject thread ATTR_UNUSED) {
  LOG(FATAL) << "JVM_SuspendThread is not implemented";
}

JNIEXPORT void JVM_ResumeThread(JNIEnv *env ATTR_UNUSED, jobject thread ATTR_UNUSED) {
  Thread* currentThread = Thread::Current();
  MutexLock mu(currentThread, *Locks::suspendMutex);
  Locks::suspendCond->Broadcast(currentThread);
}

JNIEXPORT void JVM_SetThreadPriority(JNIEnv *env, jobject javaThread, jint prio) {
  VLOG(thread) << "Java_Thread_nativeSetPriority";
  ScopedObjectAccess soa(env);
  Thread* self = Thread::Current();
  MutexLock mu(self, *Locks::threadListLock);
  Thread* thread = Thread::FromManagedThread(env, javaThread);
  if (thread != nullptr) {
    thread->SetNativePriority(prio);
  }
}

JNIEXPORT void JVM_Yield(JNIEnv *env ATTR_UNUSED, jclass threadClass ATTR_UNUSED) {
  (void)sched_yield(); // for performance not to check result
}

JNIEXPORT void JVM_Sleep(JNIEnv *env, jclass threadClass ATTR_UNUSED, jlong millis) {
  if (millis == 0) {
    if (Thread::Current()->Interrupted()) {
      jclass ecls = env->FindClass("java/lang/InterruptedException");
      env->ThrowNew(ecls, "Exception InterruptedException in JVM_Sleep");
    }
    return;
  }
  Thread::Current()->Sleep(nullptr, millis, 0);
}

JNIEXPORT jobject JVM_CurrentThread(JNIEnv *env, jclass threadClass ATTR_UNUSED) {
  VLOG(thread) << "Java_Thread_currentThread call";
  jobject retval = Thread::Current()->GetPeer();
  return env->NewLocalRef(retval);
}

JNIEXPORT jint JVM_CountStackFrames(JNIEnv *env, jobject javaThread) {
  // implementation for this function need refactor later
  jint stackSize = 0;
  pid_t currTid = Thread::Current()->GetTid();
  pid_t tid = currTid;
  Thread* thread = Thread::FromManagedThread(env, javaThread);
  if (thread != nullptr) {
    tid = thread->GetTid();
  } else {
    LOG(ERROR) << __FUNCTION__ << ":thread is null";
    return stackSize;
  }
  if (tid == currTid) {
    std::vector<UnwindContext> uwContextStack;
    MapleStack::FastRecordCurrentJavaStack(uwContextStack);
    stackSize = uwContextStack.size();
  } else {
    std::vector<maple_native_stack_item> stacks;
    maple::GetJavaStack(tid, stacks);
    stackSize = stacks.size();
  }
  return stackSize;
}

JNIEXPORT void JVM_Interrupt(JNIEnv *env, jobject javaThread) {
  VLOG(thread) << "JVM_Interrupt";
  Thread* self = Thread::Current();
  ScopedThreadSuspension sts(self, kWaiting);
  MutexLock mu(self, *Locks::threadListLock);
  Thread* thread = Thread::FromManagedThread(env, javaThread);
  if (thread != nullptr) {
    thread->Interrupt(thread);
  }
}

JNIEXPORT jboolean JVM_IsInterrupted(JNIEnv *env, jobject javaThread, jboolean clearInterrupted) {
  VLOG(thread) << "JVM_IsInterrupted";
  if (clearInterrupted == JNI_TRUE) {
    return Thread::Current()->Interrupted();
  } else {
    Thread* self = Thread::Current();
    ScopedThreadSuspension sts(self, kWaiting);
    MutexLock mu(self, *Locks::threadListLock);
    Thread* thread = Thread::FromManagedThread(env, javaThread);
    return (jboolean)(thread != nullptr ? thread->IsInterrupted() : JNI_FALSE);
  }
}

JNIEXPORT jboolean JVM_HoldsLock(JNIEnv *env, jclass threadClass, jobject obj) {
  jobject javaThread = JVM_CurrentThread(env, threadClass);
  VLOG(thread) << "JVM_HoldsLock";
  if (obj == nullptr) {
    jclass ecls = env->FindClass("java/lang/NullPointerException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception NullPointerException in JVM_HoldsLock");
    return JNI_FALSE;
  }
  Thread* self = Thread::Current();
  MutexLock mu(self, *Locks::threadListLock);
  Thread* thread = Thread::FromManagedThread(env, javaThread);
  if (thread == nullptr) {
    LOG(ERROR) << "thread is nullptr";
    return false;
  } else {
    return thread->HoldsLock(obj);
  }
  return JNI_FALSE;
}

JNIEXPORT void JVM_DumpAllStacks(JNIEnv *env ATTR_UNUSED, jclass unused ATTR_UNUSED) {
  UNIMPLEMENTED(FATAL) << "JVM_DumpAllStacks won't be called forever!";
}

JNIEXPORT jobjectArray JVM_GetAllThreads(JNIEnv *env ATTR_UNUSED, jclass dummy ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetAllThreads is not implemented";
  return nullptr;
}

JNIEXPORT void JVM_SetNativeThreadName(JNIEnv *env, jobject jthread, jstring name) {
  VLOG(thread) << "Java_Thread_nativeSetName";
  ScopedUtfChars nativeString(env, name);
  Thread* self = Thread::Current();
  MutexLock mu(self, *Locks::threadListLock);
  Thread* thread = Thread::FromManagedThread(env, jthread);
  if (thread != nullptr) {
    maplert::ScopedObjectAccess soa(env);
    thread->SetThreadName(nativeString.c_str());
  }
}

static uintptr_t GetRelativePc(const std::string &soName, uintptr_t pc) {
  // implementation for this function need refactor later, same with mpl_native_stack.cc
  struct TraceDataT cbData {.ip = pc, .relative_ip = 0, .name = soName.c_str()};

  int found = dl_iterate_phdr(
      [](struct dl_phdr_info *pinfo, size_t, void *data) -> int {
        struct TraceDataT *trace = reinterpret_cast<TraceDataT*>(data);

        if (strcmp(trace->name, pinfo->dlpi_name)) {
          // not equel, so got next dl_phdr_infor
          return false;
        }
        trace->relative_ip = trace->ip - pinfo->dlpi_addr;
        return true;
      },
      &cbData);

  if (found) {
    return cbData.relative_ip;
  } else {
    return pc;
  }
}

/* getStackTrace() and getAllStackTraces() method */
JNIEXPORT jobjectArray JVM_DumpThreads(JNIEnv *env, jclass threadClass ATTR_UNUSED, jobjectArray threads) {
  // implementation for this function need refactor later
  jclass stackTraceElementClass = env->FindClass("java/lang/StackTraceElement");
  jobjectArray tmp = env->NewObjectArray(1, stackTraceElementClass, NULL);
  jclass stackTraceElementArrClass = env->GetObjectClass(tmp);
  jobjectArray output = env->NewObjectArray(1, stackTraceElementArrClass, NULL);

  pid_t currTid = Thread::Current()->GetTid();
  pid_t tid = currTid;
  Thread* thread = Thread::FromManagedThread(env, env->GetObjectArrayElement(threads, 0));
  if (thread != nullptr) {
    tid  = thread->GetTid();
  } else {
    LOG(ERROR) << __FUNCTION__ << ":thread is null";
    return output;
  }
  std::vector<maple_native_stack_item> stacks;
  if (tid == currTid) {
    std::vector<UnwindContext> uwContextStack;
    MapleStack::FastRecordCurrentJavaStack(uwContextStack);
    for (auto &uwContext : uwContextStack) {
      maple_native_stack_item frame;
      if (!uwContext.IsInterpretedContext()) {
        uwContext.frame.GetJavaClassName(frame.declaringClass);
        uwContext.frame.GetJavaMethodName(frame.methodName);
        uint64_t ip = reinterpret_cast<uint64_t>(uwContext.frame.GetFramePC());
        frame.fileName = LinkerAPI::Instance().GetMFileNameByPC(reinterpret_cast<void*>(ip), false);
        frame.lineNumber = GetRelativePc(frame.fileName, reinterpret_cast<uintptr_t>(ip));
      } else {
        UnwindContextInterpEx::GetStackFrameFromUnwindContext(uwContext, frame);
      }
      stacks.push_back(frame);
    }
  } else {
    maple::GetJavaStack(tid, stacks);
  }

  if (stacks.empty()) {
    LOG(ERROR) << __FUNCTION__ << "stacks is empty" << maple::endl;
    return output;
  }

  int32_t depth = stacks.size();
  jobjectArray result = nullptr;
  jobjectArray java_traces = env->NewObjectArray(depth, stackTraceElementClass, NULL);
  if (java_traces == nullptr) {
    return output;
  }
  result = java_traces;

  jstring declaringClass, methodName, fileName;
  jint lineNumber;

  int i = 0;
  for (auto &frame : stacks) {
    declaringClass = env->NewStringUTF(frame.declaringClass.c_str());
    methodName = env->NewStringUTF(frame.methodName.c_str());
    fileName = env->NewStringUTF(frame.fileName.c_str());
    lineNumber = frame.lineNumber;
    if (declaringClass == nullptr || methodName == nullptr || fileName == nullptr) {
      LOG(ERROR) << "Create Java String instance failed, file " << frame.fileName << ", class "
                 << frame.declaringClass << ", method " << frame.methodName;
      return output;
    }

    jmethodID mId = env->GetMethodID(stackTraceElementClass, "<init>",
                                     "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");

    jobject obj = env->NewObject(stackTraceElementClass, mId, declaringClass, methodName,
                                 fileName, lineNumber);
    if (obj == nullptr) {
      return output;
    }
    env->SetObjectArrayElement(result, i, obj);
    ++i;
  }
  env->SetObjectArrayElement(output, 0, result);
  return output;
}

/*
 * java.lang.SecurityManager
 */
JNIEXPORT jclass JVM_CurrentLoadedClass(JNIEnv *env ATTR_UNUSED) {
  UNIMPLEMENTED(FATAL) << "JVM_CurrentLoadedClass won't be called forever!";
  return nullptr;
}

JNIEXPORT jobject JVM_CurrentClassLoader(JNIEnv *env ATTR_UNUSED) {
  UNIMPLEMENTED(FATAL) << "JVM_CurrentClassLoader won't be called forever!";
  return nullptr;
}

JNIEXPORT jobjectArray JVM_GetClassContext(JNIEnv *env ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetClassContext is not implemented";
  return nullptr;
}

JNIEXPORT jint JVM_ClassDepth(JNIEnv *env ATTR_UNUSED, jstring name ATTR_UNUSED) {
  UNIMPLEMENTED(FATAL) << "JVM_ClassDepth won't be called forever!";
  return 0;
}

JNIEXPORT jint JVM_ClassLoaderDepth(JNIEnv *env ATTR_UNUSED) {
  UNIMPLEMENTED(FATAL) << "JVM_ClassLoaderDepth won't be called forever!";
  return 0;
}

/*
 * java.lang.Package
 */
JNIEXPORT jstring JVM_GetSystemPackage(JNIEnv *env ATTR_UNUSED, jstring name ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetSystemPackage is not implemented";
  return nullptr;
}

JNIEXPORT jobjectArray JVM_GetSystemPackages(JNIEnv *env ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetSystemPackages is not implemented";
  return nullptr;
}

/*
 * java.io.ObjectInputStream
 */
JNIEXPORT jobject JVM_AllocateNewObject(JNIEnv *env ATTR_UNUSED, jobject obj ATTR_UNUSED,
                                        jclass currClass ATTR_UNUSED, jclass initClass ATTR_UNUSED) {
  LOG(FATAL) << "JVM_AllocateNewObject is not implemented";
  return nullptr;
}

JNIEXPORT jobject JVM_AllocateNewArray(JNIEnv *env ATTR_UNUSED, jobject obj ATTR_UNUSED,
                                       jclass currClass ATTR_UNUSED, jint length ATTR_UNUSED) {
  LOG(FATAL) << "JVM_AllocateNewArray is not implemented";
  return nullptr;
}

JNIEXPORT jobject JVM_LatestUserDefinedLoader(JNIEnv *env ATTR_UNUSED) {
  return nullptr;
}

/*
 * This function has been deprecated and should not be considered
 * part of the specified JVM interface.
 */
JNIEXPORT jclass JVM_LoadClass0(JNIEnv *env ATTR_UNUSED, jobject obj ATTR_UNUSED,
                                jclass currClass ATTR_UNUSED, jstring currClassName ATTR_UNUSED) {
  LOG(FATAL) << "JVM_LoadClass0 is not implemented";
  return nullptr;
}

char JVM_ConvertTypeToPrimitive(int vCode) {
  char res = 'N';
  switch (vCode) {
    case JVM_T_BOOLEAN:
      res = 'Z';
      break;
    case JVM_T_CHAR:
      res = 'C';
      break;
    case JVM_T_FLOAT:
      res = 'F';
      break;
    case JVM_T_DOUBLE:
      res = 'D';
      break;
    case JVM_T_BYTE:
      res = 'B';
      break;
    case JVM_T_SHORT:
      res = 'S';
      break;
    case JVM_T_INT:
      res = 'I';
      break;
    case JVM_T_LONG:
      res = 'J';
      break;
    default:
      LOG(FATAL) << "JVM_ConvertTypeToPrimitive error vCode";
      break;
  }
  return res;
}

/*
 * java.lang.reflect.Array
 */
JNIEXPORT jint JVM_GetArrayLength(JNIEnv *env, jobject arr) {
  jclass ecls = nullptr;
  if (UNLIKELY(arr == nullptr)) {
    ecls = env->FindClass("java/lang/NullPointerException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception NullPointerException in JVM_GetArrayLength");
    return 0;
  }
  jboolean isArray = MRT_IsArray(arr);
  if (UNLIKELY(isArray == false)) {
    ecls = env->FindClass("java/lang/IllegalArgumentException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception IllegalArgumentException in JVM_GetArrayLength");
    return 0;
  }
  jint length = MRT_GetArrayElementCount(reinterpret_cast<jarray>(arr));
  return length;
}

JNIEXPORT jobject JVM_GetArrayElement(JNIEnv *env, jobject arr, jint index) {
  jclass ecls = nullptr;
  if (UNLIKELY(arr == nullptr)) {
    ecls = env->FindClass("java/lang/NullPointerException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception NullPointerException in JVM_GetArrayElement");
    return JNI_FALSE;
  }
  jboolean isArray = MRT_IsArray(arr);
  if (UNLIKELY(isArray == false)) {
    ecls = env->FindClass("java/lang/IllegalArgumentException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception IllegalArgumentException in JVM_GetArrayElement");
    return JNI_FALSE;
  }
  jint length = MRT_GetArrayElementCount(reinterpret_cast<jarray>(arr));
  if (UNLIKELY(index < 0 || index >= length)) {
    ecls = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception ArrayIndexOutOfBoundsException in JVM_GetArrayElement");
    return JNI_FALSE;
  }
  maplert::ScopedObjectAccess soa;
  jobject arrayElement = MRT_GetArrayElement(reinterpret_cast<jobjectArray>(arr), static_cast<jsize>(index), true);
  return maplert::MRT_JNI_AddLocalReference(env, arrayElement);
}

JNIEXPORT jvalue JVM_GetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jint wCode) {
  maplert::ScopedObjectAccess soa;
  jclass ecls = nullptr;
  jvalue javaValue;
  if (UNLIKELY(arr == nullptr)) {
    ecls = env->FindClass("java/lang/NullPointerException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception NullPointerException in JVM_GetPrimitiveArrayElement");
    javaValue.l = JNI_FALSE;
    return javaValue;
  }
  jarray jArray = static_cast<jarray>(arr);
  jboolean isPrimitiveArray = MRT_IsPrimitveArray(jArray);
  jboolean isMultiArray = MRT_IsMultiDimArray(jArray);
  if (UNLIKELY(isMultiArray == true || isPrimitiveArray == false)) {
    ecls = env->FindClass("java/lang/IllegalArgumentException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception IllegalArgumentException in JVM_GetPrimitiveArrayElement");
    javaValue.l = JNI_FALSE;
    return javaValue;
  }
  jint length = MRT_GetArrayElementCount(jArray);
  if (UNLIKELY(index < 0 || index >= length)) {
    ecls = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception ArrayIndexOutOfBoundsException in JVM_GetPrimitiveArrayElement");
    javaValue.l = JNI_FALSE;
    return javaValue;
  }
  jclass arrClass = MRT_ReflectGetObjectClass(jArray);
  jclass arrCompClass = MRT_ReflectClassGetComponentType(arrClass);
  char arrType = MRT_GetPrimitiveType(arrCompClass);
  char wCodeType = JVM_ConvertTypeToPrimitive(wCode);
  if (UNLIKELY(wCodeType != arrType)) {
    javaValue = MRT_GetPrimitiveArrayElement(jArray, index, arrType);
    jvalue dstValue;
    jboolean widenTest = MRT_TypeWidenConvertCheck(arrType, wCodeType, javaValue, dstValue);
    if (widenTest == false) {
      ecls = env->FindClass("java/lang/IllegalArgumentException");
      CHECK(ecls != nullptr) << "ecls is nullptr";
      env->ThrowNew(ecls, "Exception IllegalArgumentException in JVM_GetPrimitiveArrayElement");
      dstValue.l = JNI_FALSE;
      return dstValue;
    } else {
      return dstValue;
    }
  }
  javaValue = MRT_GetPrimitiveArrayElement(jArray, index, arrType);
  return javaValue;
}

JNIEXPORT void JVM_SetArrayElement(JNIEnv *env, jobject arr, jint index, jobject val) {
  maplert::ScopedObjectAccess soa;
  jclass ecls = nullptr;
  if (UNLIKELY(arr == nullptr)) {
    ecls = env->FindClass("java/lang/NullPointerException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception NullPointerException in JVM_SetArrayElement");
    return;
  }
  jboolean isArray = MRT_IsArray(arr);
  if (UNLIKELY(isArray == false)) {
    ecls = env->FindClass("java/lang/IllegalArgumentException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception IllegalArgumentException in JVM_SetArrayElement");
    return;
  }
  jint length = MRT_GetArrayElementCount(reinterpret_cast<jarray>(arr));
  if (UNLIKELY(index < 0 || index >= length)) {
    ecls = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception ArrayIndexOutOfBoundsException in JVM_SetArrayElement");
    return;
  }
  jclass arrClass = MRT_ReflectGetObjectClass(arr);
  jclass arrCompClass = MRT_ReflectClassGetComponentType(arrClass);
  if (!MRT_IsPrimitveArray(arr)) {
    char arrType = MRT_GetPrimitiveTypeFromBoxType(arrCompClass);
    if (val != nullptr) {
      jboolean valueCheck = MRT_TypeWidenConvertCheckObject(val);
      jclass valueClass = MRT_ReflectGetObjectClass(val);
      char valType = MRT_GetPrimitiveTypeFromBoxType(valueClass);
      if (valueCheck != true && arrType != valType) {
        ecls = env->FindClass("java/lang/IllegalArgumentException");
        CHECK(ecls != nullptr) << "ecls is nullptr";
        env->ThrowNew(ecls, "Exception IllegalArgumentException in JVM_SetArrayElement");
        return;
      }
    }
  } else {
    if (UNLIKELY(val == nullptr)) {
      ecls = env->FindClass("java/lang/IllegalArgumentException");
      CHECK(ecls != nullptr) << "ecls is nullptr";
      env->ThrowNew(ecls, "Exception IllegalArgumentException in JVM_SetArrayElement");
      return;
    }
    jclass valueClass = MRT_ReflectGetObjectClass(val);
    char valType = MRT_GetPrimitiveTypeFromBoxType(valueClass);
    char arrType = MRT_GetPrimitiveType(arrCompClass);
    if (UNLIKELY(valType != arrType)) {
      jvalue v;
      v.l = val;
      jvalue tmpCheck = v;
      jboolean widenTest = MRT_TypeWidenConvertCheck(valType, arrType, v, tmpCheck);
      if (widenTest == false) {
        ecls = env->FindClass("java/lang/IllegalArgumentException");
        CHECK(ecls != nullptr) << "ecls is nullptr";
        env->ThrowNew(ecls, "Exception IllegalArgumentException in JVM_SetArrayElement");
        return;
      }
    }
  }
  MRT_SetArrayElement(reinterpret_cast<jobjectArray>(arr), static_cast<jsize>(index), val);
  return;
}

JNIEXPORT void JVM_SetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jvalue v, unsigned char vCode) {
  maplert::ScopedObjectAccess soa;
  jclass ecls = nullptr;
  if (UNLIKELY(arr == nullptr)) {
    ecls = env->FindClass("java/lang/NullPointerException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception NullPointerException in JVM_SetPrimitiveArrayElement");
    return;
  }
  jarray jArray = static_cast<jarray>(arr);
  jboolean isPrimitiveArray = MRT_IsPrimitveArray(jArray);
  jboolean isMultiArray = MRT_IsMultiDimArray(jArray);
  if (UNLIKELY(isMultiArray == true || isPrimitiveArray == false)) {
    ecls = env->FindClass("java/lang/IllegalArgumentException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception IllegalArgumentException in JVM_SetPrimitiveArrayElement");
    return;
  }
  jint length = MRT_GetArrayElementCount(jArray);
  if (UNLIKELY(index < 0 || index >= length)) {
    ecls = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception ArrayIndexOutOfBoundsException in JVM_SetPrimitiveArrayElement");
    return;
  }
  jclass arrClass = MRT_ReflectGetObjectClass(jArray);
  jclass arrCompClass = MRT_ReflectClassGetComponentType(arrClass);
  char arrType = MRT_GetPrimitiveType(arrCompClass);
  char vCodeType = JVM_ConvertTypeToPrimitive(static_cast<int>(vCode));
  if (UNLIKELY(vCodeType != arrType)) {
    jvalue tmpCheck = v;
    jboolean widenTest = MRT_TypeWidenConvertCheck(vCodeType, arrType, v, tmpCheck);
    if (widenTest == false) {
      ecls = env->FindClass("java/lang/IllegalArgumentException");
      CHECK(ecls != nullptr) << "ecls is nullptr";
      env->ThrowNew(ecls, "Exception IllegalArgumentException in JVM_SetPrimitiveArrayElement");
      return;
    } else {
      MRT_SetPrimitiveArrayElement(jArray, index, tmpCheck, arrType);
      return;
    }
  }
  MRT_SetPrimitiveArrayElement(jArray, index, v, arrType);
  return;
}

JNIEXPORT jobject JVM_NewArray(JNIEnv *env, jclass eltClass, jint length) {
  VLOG(thread) << "JVM_NewArray";
  maplert::ScopedObjectAccess soa;
  jclass ecls = nullptr;
  if (UNLIKELY(eltClass == nullptr)) {
    ecls = env->FindClass("java/lang/NullPointerException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception NullPointerException in JVM_NewArray");
    return nullptr;
  }
  if (UNLIKELY(length < 0)) {
    std::string msg = std::to_string(length);
    ecls = env->FindClass("java/lang/NegativeArraySizeException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception NegativeArraySizeException in JVM_NewArray");
    return nullptr;
  }
  jobject obj;
  if (MRT_ReflectClassIsPrimitive(eltClass)) {
    jint eltSize = MRT_ReflectGetObjSize(eltClass);
    obj = MRT_NewArray(length, eltClass, eltSize);
  } else {
    obj = MRT_NewObjArray(length, eltClass, nullptr);
  }
  return maplert::MRT_JNI_AddLocalReference(env, obj);
}

JNIEXPORT jobject JVM_NewMultiArray(JNIEnv *env, jclass eltClass, jintArray dim) {
  maplert::ScopedObjectAccess soa;
  jclass ecls = nullptr;
  if (UNLIKELY(dim == nullptr)) {
    ecls = env->FindClass("java/lang/NullPointerException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception NullPointerException in JVM_NewMultiArray");
    return nullptr;
  }
  jint dimCount = MRT_GetArrayElementCount((jarray)dim);
  if (UNLIKELY(dimCount == 0)) {
    ecls = env->FindClass("java/lang/IllegalArgumentException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception IllegalArgumentException in JVM_NewMultiArray");
    return nullptr;
  }
  jint *dimArray = reinterpret_cast<jint*>(MRT_JavaArrayToCArray(dim));
  jclass arrayClass = eltClass;
  for (jint i = 0; i < dimCount; i++) {
    jint dimension = dimArray[i];
    arrayClass = MRT_ReflectGetOrCreateArrayClassObj(arrayClass);
    if (UNLIKELY(dimension < 0)) {
      ecls = env->FindClass("java/lang/NegativeArraySizeException");
      CHECK(ecls != nullptr) << "ecls is nullptr";
      env->ThrowNew(ecls, stringutils::Format("Dimension %d: %d", i, dimension).c_str());
      return nullptr;
    }
  }
  return  maplert::MRT_JNI_AddLocalReference(env, MRT_RecursiveCreateMultiArray(arrayClass, 0, dimCount, dimArray));
}

/*
 * java.lang.Class and java.lang.ClassLoader
 */
#define JVM_CALLER_DEPTH -1

/*
 * Returns the immediate caller class of the native method invoking
 * JVM_GetCallerClass.  The Method.invoke and other frames due to
 * reflection machinery are skipped.
 *
 * The depth parameter must be -1 (JVM_DEPTH). The caller is expected
 * to be marked with sun.reflect.CallerSensitive.  The JVM will throw
 * an error if it is not marked propertly.
 */
JNIEXPORT jclass JVM_GetCallerClass(JNIEnv *env ATTR_UNUSED, int depth) {
  std::vector<UnwindContext> uwContextStack;
  uwContextStack.reserve(10);
#ifdef __OPENJDK__
  // set stack level to get caller class <__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B>
  uint32_t level = 2;
#else
  uint32_t level = 0;
#endif
  if (depth != -1) {
    level = depth;
  }
  MapleStack::FastRecordCurrentJavaStack(uwContextStack, level + 1);
  jclass klass = nullptr;
  uint32_t size = uwContextStack.size();
  if (size > level && !uwContextStack.empty()) {
    if (!uwContextStack[level].IsInterpretedContext()) {
      klass = uwContextStack[level].frame.GetDeclaringClass();
    } else {
      klass = *UnwindContextInterpEx::GetDeclaringClassFromUnwindContext(uwContextStack[level]);
    }
  }
  return klass;
}

class JavaPrimitiveType{
 public:
  const char *name;
  jclass (*func)(void);

  JavaPrimitiveType(const char *name, jclass (*func)(void)) {
    this->name = name;
    this->func = func;
  }

  ~JavaPrimitiveType() {
  }
};

// Map BasicType to Java type name
JavaPrimitiveType primitiveTypes[11] = {
    JavaPrimitiveType("boolean", MRT_GetPrimitiveClassJboolean),
    JavaPrimitiveType("char", MRT_GetPrimitiveClassJchar),
    JavaPrimitiveType("float", MRT_GetPrimitiveClassJfloat),
    JavaPrimitiveType("double", MRT_GetPrimitiveClassJdouble),
    JavaPrimitiveType("byte", MRT_GetPrimitiveClassJbyte),
    JavaPrimitiveType("short", MRT_GetPrimitiveClassJshort),
    JavaPrimitiveType("int", MRT_GetPrimitiveClassJint),
    JavaPrimitiveType("long", MRT_GetPrimitiveClassJlong),
    JavaPrimitiveType("void", MRT_GetPrimitiveClassVoid),
    JavaPrimitiveType("object", MRT_GetClassObject),
    JavaPrimitiveType("array", MRT_GetClassObject)
};

/*
 * Find primitive classes
 * utf: class name
 */
JNIEXPORT jclass JVM_FindPrimitiveClass(JNIEnv *env ATTR_UNUSED, const char *utf) {
  // 11 is primitiveTypes's size
  for (int i = 0; i < 11; i++) {
    if (strcmp(primitiveTypes[i].name, utf) == 0) {
      return primitiveTypes[i].func();
    }
  }
  return nullptr;
}

/*
 * Link the class
 */
JNIEXPORT void JVM_ResolveClass(JNIEnv *env ATTR_UNUSED, jclass cls ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ResolveClass is not implemented";
}

/*
 * Find a class from a boot class loader. Returns NULL if class not found.
 */
JNIEXPORT jclass JVM_FindClassFromBootLoader(JNIEnv *env ATTR_UNUSED, const char *name ATTR_UNUSED) {
  // not implemented, implemented in low level codes, here just return null.
  return nullptr;
}

/*
 * Find a class from a given class loader.  Throws ClassNotFoundException.
 *  name:   name of class
 *  init:   whether initialization is done
 *  loader: class loader to look up the class. This may not be the same as the caller's
 *          class loader.
 *  caller: initiating class. The initiating class may be NULL when a security
 *          manager is not installed.
 */
JNIEXPORT jclass JVM_FindClassFromCaller(JNIEnv *env, const char *name, jboolean init, jobject loader,
                                         jclass caller ATTR_UNUSED) {
  // precondition: suppose caller == NULL
  ScopedObjectAccess soa(env);
  if (name == nullptr) {
    ThrowNewExceptionF(env, "java/lang/NullPointerException", "class name is null");
    return nullptr;
  }
  // We need to validate and convert the name (from x.y.z to x/y/z).  This
  // is especially handy for array types, since we want to avoid
  // auto-generating bogus array classes.
  if (!NameValidator::IsValidJniClassName(name)) {
    ThrowNewExceptionF(env, "java/lang/ClassNotFoundException",
                       "Invalid name: %s", name);
    return nullptr;
  }
  std::string descriptor(name);
  ClassLinker* class_linker = Runtime::Current()->GetClassLinker();
  jclass c = class_linker->FindClass(ThreadForEnv(env), descriptor.c_str(), loader);
  if (UNLIKELY(c == nullptr && !init)) { // Only for ETS test, 'cause ETS uses Class.forName(, false, )
    c = class_linker->FindMappedClass(ThreadForEnv(env), descriptor.c_str(), loader);
  }
  if (UNLIKELY(c == nullptr || (!init && !MRT_ClassIsSuperClassValid(c)))) {
    ThrowNewExceptionF(env, "java/lang/ClassNotFoundException", name);
    return nullptr;
  }
  if (init) {
    (void)class_linker->EnsureInitialized(Thread::Current(), c, true, true);
  }
  return c;
}

/*
 * Find a class from a given class loader. Throw ClassNotFoundException
 * or NoClassDefFoundError depending on the value of the last
 * argument.
 */
JNIEXPORT jclass JVM_FindClassFromClassLoader(JNIEnv *env ATTR_UNUSED, const char *name ATTR_UNUSED,
                                              jboolean init ATTR_UNUSED, jobject loader ATTR_UNUSED,
                                              jboolean throwError ATTR_UNUSED) {
  LOG(FATAL) << "JVM_FindClassFromClassLoader is not implemented";
  return nullptr;
}

/*
 * Find a class from a given class.
 */
JNIEXPORT jclass JVM_FindClassFromClass(JNIEnv *env ATTR_UNUSED, const char *name ATTR_UNUSED,
                                        jboolean init ATTR_UNUSED, jclass from ATTR_UNUSED) {
  LOG(FATAL) << "JVM_FindClassFromClass is not implemented";
  return nullptr;
}

/* Find a loaded class cached by the VM */
JNIEXPORT jclass JVM_FindLoadedClass(JNIEnv *env, jobject loader ATTR_UNUSED, jstring name) {
  maplert::ScopedObjectAccess soa;
  int length = (env)->GetStringLength(name);
  if (length == 0) {
    jclass ecls = env->FindClass("java/lang/ClassNotFoundException");
    CHECK(ecls != nullptr) << "ecls is nullptr";
    env->ThrowNew(ecls, "Exception ClassNotFoundException in JVM_FindLoadedClass");
    return nullptr;
  }
  jboolean isCopy = JNI_FALSE;
  char* className = MRT_GetStringMUTFChars(name, &isCopy);
  jclass strClz = (jclass)env->FindClass(className);
  if (isCopy) {
    MRT_ReleaseStringUTFChars(name, className);
  }
  return strClz;
}

/* Define a class */
JNIEXPORT jclass JVM_DefineClass(JNIEnv *env ATTR_UNUSED, const char *name ATTR_UNUSED,
    jobject loader ATTR_UNUSED, const jbyte *buf ATTR_UNUSED, jsize len ATTR_UNUSED,
    jobject pd ATTR_UNUSED) {
  LOG(FATAL) << "JVM_DefineClass is not implemented";
  return nullptr;
}

/* Define a class with a source (added in JDK1.5) */
JNIEXPORT jclass JVM_DefineClassWithSource(JNIEnv *env ATTR_UNUSED, const char *name ATTR_UNUSED,
    jobject loader ATTR_UNUSED, const jbyte *buf ATTR_UNUSED, jsize len ATTR_UNUSED,
    jobject pd ATTR_UNUSED, const char *source ATTR_UNUSED) {
  LOG(FATAL) << "JVM_DefineClassWithSource is not implemented";
  return nullptr;
}

/*
 * Reflection support functions
 */
JNIEXPORT jstring JVM_GetClassName(JNIEnv *env, jclass cls) {
  maplert::ScopedObjectAccess soa;
  jstring className = MRT_ReflectClassGetName(cls);
  return (jstring)maplert::MRT_JNI_AddLocalReference(env, className);
}

JNIEXPORT jobjectArray JVM_GetClassInterfaces(JNIEnv *env, jclass cls) {
  maplert::ScopedObjectAccess soa;
  jobjectArray classInterfaces = MRT_ReflectClassGetInterfaces(cls);
  return static_cast<jobjectArray>(maplert::MRT_JNI_AddLocalReference(env, classInterfaces));
}

JNIEXPORT jboolean JVM_IsInterface(JNIEnv *env ATTR_UNUSED, jclass cls) {
  jboolean isInterface = MRT_ReflectClassIsInterface(cls);
  return isInterface;
}

JNIEXPORT jobjectArray JVM_GetClassSigners(JNIEnv *env ATTR_UNUSED, jclass cls ATTR_UNUSED) {
  // nullptr for no-signers
  return nullptr;
}

JNIEXPORT void JVM_SetClassSigners(JNIEnv *env ATTR_UNUSED, jclass cls ATTR_UNUSED,
                                   jobjectArray signers ATTR_UNUSED) {
  // do nothing
}

JNIEXPORT jobject JVM_GetProtectionDomain(JNIEnv *env ATTR_UNUSED, jclass cls ATTR_UNUSED) {
  // nullptr for no protection domains
  return nullptr;
}

JNIEXPORT jboolean JVM_IsArrayClass(JNIEnv *env ATTR_UNUSED, jclass cls) {
  jboolean arrayFlag = MRT_ReflectClassIsArray(cls);
  return arrayFlag;
}

JNIEXPORT jboolean JVM_IsPrimitiveClass(JNIEnv *env ATTR_UNUSED, jclass cls) {
  jboolean isPrimitive = MRT_ReflectClassIsPrimitive(cls);
  return isPrimitive;
}

JNIEXPORT jclass JVM_GetComponentType(JNIEnv *env, jclass cls) {
  maplert::ScopedObjectAccess soa;
  jclass componentType = MRT_ReflectClassGetComponentType(cls);
  return (jclass)maplert::MRT_JNI_AddLocalReference(env, componentType);
}

JNIEXPORT jint JVM_GetClassModifiers(JNIEnv *env ATTR_UNUSED, jclass cls) {
  jint classModifiers = MRT_ReflectClassGetModifiers(cls);
  return classModifiers;
}

JNIEXPORT jobjectArray JVM_GetDeclaredClasses(JNIEnv *env, jclass ofClass) {
  maplert::ScopedObjectAccess soa;
  jobjectArray declaredClasses = MRT_ReflectClassGetDeclaredClasses(ofClass);
  return static_cast<jobjectArray>(maplert::MRT_JNI_AddLocalReference(env, declaredClasses));
}

JNIEXPORT jclass JVM_GetDeclaringClass(JNIEnv *env, jclass ofClass) {
  maplert::ScopedObjectAccess soa;
  jclass declaring = MRT_ReflectClassGetDeclaringClass(ofClass);
  return static_cast<jclass>(maplert::MRT_JNI_AddLocalReference(env, declaring));
}

/* Generics support (JDK 1.5) */
JNIEXPORT jstring JVM_GetClassSignature(JNIEnv *env ATTR_UNUSED, jclass cls ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetClassSignature is not implemented";
  return nullptr;
}

/* Annotations support (JDK 1.5) */
JNIEXPORT jbyteArray JVM_GetClassAnnotations(JNIEnv *env ATTR_UNUSED, jclass cls ATTR_UNUSED) {
  NEVERCALLTHIS(FATAL) << "JVM_GetClassAnnotations should not be called";
  return nullptr;
}

/* Type use annotations support (JDK 1.8) */
JNIEXPORT jbyteArray JVM_GetClassTypeAnnotations(JNIEnv *env ATTR_UNUSED, jclass cls ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetClassTypeAnnotations is not implemented";
  return nullptr;
}

JNIEXPORT jbyteArray JVM_GetFieldTypeAnnotations(JNIEnv *env ATTR_UNUSED, jobject field ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetFieldTypeAnnotations is not implemented";
  return nullptr;
}

JNIEXPORT jbyteArray JVM_GetMethodTypeAnnotations(JNIEnv *env ATTR_UNUSED, jobject method ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodTypeAnnotations is not implemented";
  return nullptr;
}

/*
 * New (JDK 1.4) reflection implementation
 */
JNIEXPORT jobjectArray
JVM_GetClassDeclaredMethods(JNIEnv *env, jclass ofClass, jboolean publicOnly) {
  maplert::ScopedObjectAccess soa;
  jobject methods = MRT_ReflectClassGetDeclaredMethods(ofClass, publicOnly);
  return (jobjectArray)maplert::MRT_JNI_AddLocalReference(env, methods);
}

JNIEXPORT jobjectArray
JVM_GetClassDeclaredFields(JNIEnv *env, jclass ofClass, jboolean publicOnly) {
  maplert::ScopedObjectAccess soa;
  jobject fields = MRT_ReflectClassGetDeclaredFields(ofClass, publicOnly);
  return (jobjectArray)maplert::MRT_JNI_AddLocalReference(env, fields);
}

JNIEXPORT jobjectArray
JVM_GetClassDeclaredConstructors(JNIEnv *env, jclass ofClass, jboolean publicOnly) {
  maplert::ScopedObjectAccess soa;
  jobject constructors = MRT_ReflectClassGetDeclaredConstructors(ofClass, publicOnly);
  return (jobjectArray)maplert::MRT_JNI_AddLocalReference(env, constructors);
}

/* Differs from JVM_GetClassModifiers in treatment of inner classes.
   This returns the access flags for the class as specified in the
   class file rather than searching the InnerClasses attribute (if
   present) to find the source-level access flags. Only the values of
   the low 13 bits (i.e., a mask of 0x1FFF) are guaranteed to be
   valid. */
JNIEXPORT jint JVM_GetClassAccessFlags(JNIEnv *env ATTR_UNUSED, jclass cls) {
  return MRT_ReflectClassGetAccessFlags(cls);
}

/* The following two reflection routines are still needed due to startup time issues */
/*
 * java.lang.reflect.Method
 */
JNIEXPORT jobject JVM_InvokeMethod(JNIEnv *env ATTR_UNUSED, jobject method ATTR_UNUSED,
                                   jobject obj ATTR_UNUSED, jobjectArray args0 ATTR_UNUSED) {
  LOG(FATAL) << "JVM_InvokeMethod is not implemented";
  return nullptr;
}

/*
 * java.lang.reflect.Constructor
 */
JNIEXPORT jobject JVM_NewInstanceFromConstructor(JNIEnv *env ATTR_UNUSED, jobject c ATTR_UNUSED,
                                                 jobjectArray args0 ATTR_UNUSED) {
  LOG(FATAL) << "JVM_NewInstanceFromConstructor is not implemented";
  return nullptr;
}

/*
 * Constant pool access; currently used to implement reflective access to annotations (JDK 1.5)
 */
JNIEXPORT jobject JVM_GetClassConstantPool(JNIEnv *env ATTR_UNUSED, jclass cls ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetClassConstantPool is not implemented";
  return nullptr;
}

JNIEXPORT jint JVM_ConstantPoolGetSize(JNIEnv *env ATTR_UNUSED, jobject unused ATTR_UNUSED,
                                       jobject jcpool ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetSize is not implemented";
  return 0;
}

JNIEXPORT jclass JVM_ConstantPoolGetClassAt(JNIEnv *env ATTR_UNUSED, jobject unused ATTR_UNUSED,
                                            jobject jcpool ATTR_UNUSED, jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetClassAt is not implemented";
  return nullptr;
}

JNIEXPORT jclass JVM_ConstantPoolGetClassAtIfLoaded(JNIEnv *env ATTR_UNUSED, jobject unused ATTR_UNUSED,
                                                    jobject jcpool ATTR_UNUSED, jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetClassAtIfLoaded is not implemented";
  return nullptr;
}

JNIEXPORT jobject JVM_ConstantPoolGetMethodAt(JNIEnv *env ATTR_UNUSED, jobject unused ATTR_UNUSED,
                                              jobject jcpool ATTR_UNUSED, jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetMethodAt is not implemented";
  return nullptr;
}

JNIEXPORT jobject JVM_ConstantPoolGetMethodAtIfLoaded(JNIEnv *env ATTR_UNUSED, jobject unused ATTR_UNUSED,
                                                      jobject jcpool ATTR_UNUSED, jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetMethodAtIfLoaded is not implemented";
  return nullptr;
}

JNIEXPORT jobject JVM_ConstantPoolGetFieldAt(JNIEnv *env ATTR_UNUSED, jobject unused ATTR_UNUSED,
                                             jobject jcpool ATTR_UNUSED, jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetFieldAt is not implemented";
  return nullptr;
}

JNIEXPORT jobject JVM_ConstantPoolGetFieldAtIfLoaded(JNIEnv *env ATTR_UNUSED, jobject unused ATTR_UNUSED,
                                                     jobject jcpool ATTR_UNUSED, jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetFieldAtIfLoaded is not implemented";
  return nullptr;
}

JNIEXPORT jobjectArray JVM_ConstantPoolGetMemberRefInfoAt(JNIEnv *env ATTR_UNUSED,
                                                          jobject unused ATTR_UNUSED,
                                                          jobject jcpool ATTR_UNUSED,
                                                          jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetMemberRefInfoAt is not implemented";
  return nullptr;
}

JNIEXPORT jint JVM_ConstantPoolGetIntAt(JNIEnv *env ATTR_UNUSED, jobject unused ATTR_UNUSED,
                                        jobject jcpool ATTR_UNUSED, jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetIntAt is not implemented";
  return 0;
}

JNIEXPORT jlong JVM_ConstantPoolGetLongAt(JNIEnv *env ATTR_UNUSED, jobject unused ATTR_UNUSED,
                                          jobject jcpool ATTR_UNUSED, jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetLongAt is not implemented";
  return 0L;
}

JNIEXPORT jfloat JVM_ConstantPoolGetFloatAt(JNIEnv *env ATTR_UNUSED, jobject unused ATTR_UNUSED,
                                            jobject jcpool ATTR_UNUSED, jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetFloatAt is not implemented";
  return 0;
}

JNIEXPORT jdouble JVM_ConstantPoolGetDoubleAt(JNIEnv *env ATTR_UNUSED, jobject unused ATTR_UNUSED,
                                              jobject jcpool ATTR_UNUSED, jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetDoubleAt is not implemented";
  return 0;
}

JNIEXPORT jstring JVM_ConstantPoolGetStringAt(JNIEnv *env ATTR_UNUSED, jobject unused ATTR_UNUSED,
                                              jobject jcpool ATTR_UNUSED, jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetStringAt is not implemented";
  return nullptr;
}

JNIEXPORT jstring JVM_ConstantPoolGetUTF8At(JNIEnv *env ATTR_UNUSED, jobject unused ATTR_UNUSED,
                                            jobject jcpool ATTR_UNUSED, jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ConstantPoolGetUTF8At is not implemented";
  return nullptr;
}

/*
 * Parameter reflection
 */
JNIEXPORT jobjectArray JVM_GetMethodParameters(JNIEnv *env ATTR_UNUSED, jobject method ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodParameters is not implemented";
  return nullptr;
}

/*
 * java.security.*
 */
JNIEXPORT jobject JVM_DoPrivileged(JNIEnv *env ATTR_UNUSED, jclass cls ATTR_UNUSED,
    jobject action ATTR_UNUSED, jobject context ATTR_UNUSED, jboolean wrapException ATTR_UNUSED) {
  // art does not use java.security.AccessController.java
  LOG(FATAL) << "JVM_DoPrivileged is not implemented";
  return nullptr;
}

JNIEXPORT jobject JVM_GetInheritedAccessControlContext(JNIEnv *env ATTR_UNUSED, jclass cls ATTR_UNUSED) {
  // art does not use java.security.AccessController.java
  LOG(FATAL) << "JVM_GetInheritedAccessControlContext is not implemented";
  return nullptr;
}

JNIEXPORT jobject JVM_GetStackAccessControlContext(JNIEnv *env ATTR_UNUSED, jclass cls ATTR_UNUSED) {
  // art does not use java.security.AccessController.java
  LOG(FATAL) << "JVM_GetStackAccessControlContext is not implemented";
  return nullptr;
}

/*
 * Signal support, used to implement the shutdown sequence.  Every VM must
 * support JVM_SIGINT and JVM_SIGTERM, raising the former for user interrupts
 * (^C) and the latter for external termination (kill, system shutdown, etc.).
 * Other platform-dependent signal values may also be supported.
 */
JNIEXPORT void * JVM_RegisterSignal(jint sig ATTR_UNUSED, void *handler ATTR_UNUSED) {
  // no need to implement
  LOG(FATAL) << "JVM_RegisterSignal is not implemented";
  return nullptr;
}

JNIEXPORT jboolean JVM_RaiseSignal(jint sig ATTR_UNUSED) {
  // no need to implement
  LOG(FATAL) << "JVM_RaiseSignal is not implemented";
  return JNI_FALSE;;
}

JNIEXPORT jint JVM_FindSignal(const char *name ATTR_UNUSED) {
  // no need to implement
  LOG(FATAL) << "JVM_FindSignal is not implemented";
  return 0;
}

/*
 * Retrieve the assertion directives for the specified class.
 */
JNIEXPORT jboolean JVM_DesiredAssertionStatus(JNIEnv *env ATTR_UNUSED, jclass unused ATTR_UNUSED,
                                              jclass cls ATTR_UNUSED) {
  LOG(FATAL) << "JVM_DesiredAssertionStatus is not implemented";
  return JNI_FALSE;
}

/*
 * Retrieve the assertion directives from the VM.
 */
JNIEXPORT jobject JVM_AssertionStatusDirectives(JNIEnv *env ATTR_UNUSED, jclass unused ATTR_UNUSED) {
  // LOG(FATAL) << "JVM_AssertionStatusDirectives is not implemented";
  return nullptr;
}

/*
 * java.util.concurrent.atomic.AtomicLong
 */
JNIEXPORT jboolean JVM_SupportsCX8(void) {
  // 64bit machine operations on long value are atomic
  return JNI_TRUE;
}

/*
 * com.sun.dtrace.jsdt support
 */
#define JVM_TRACING_DTRACE_VERSION 1

/*
 * Structure to pass one probe description to JVM
 */
struct JVM_DTraceProbe {
  jmethodID method;
  jstring   function;
  jstring   name;
  void*            reserved[4];     // for future use
};

/**
 * Encapsulates the stability ratings for a DTrace provider field
 */
struct JVM_DTraceInterfaceAttributes {
  jint nameStability;
  jint dataStability;
  jint dependencyClass;
};

/*
 * Structure to pass one provider description to JVM
 */
struct JVM_DTraceProvider {
  jstring                       name;
  JVM_DTraceProbe*              probes;
  jint                          probe_count;
  JVM_DTraceInterfaceAttributes providerAttributes;
  JVM_DTraceInterfaceAttributes moduleAttributes;
  JVM_DTraceInterfaceAttributes functionAttributes;
  JVM_DTraceInterfaceAttributes nameAttributes;
  JVM_DTraceInterfaceAttributes argsAttributes;
  void*                         reserved[4]; // for future use
};

/*
 * Get the version number the JVM was built with
 */
JNIEXPORT jint JVM_DTraceGetVersion(JNIEnv* env ATTR_UNUSED) {
  LOG(FATAL) << "JVM_DTraceGetVersion is not implemented";
  return 0;
}

/*
 * Register new probe with given signature, return global handle
 *
 * The version passed in is the version that the library code was
 * built with.
 */
JNIEXPORT jlong JVM_DTraceActivate(JNIEnv* env ATTR_UNUSED, jint version ATTR_UNUSED,
                                   jstring moduleName ATTR_UNUSED, jint providersCount ATTR_UNUSED,
                                   JVM_DTraceProvider* providers ATTR_UNUSED) {
  LOG(FATAL) << "JVM_DTraceActivate is not implemented";
  return 0L;
}

/*
 * Check JSDT probe
 */
JNIEXPORT jboolean JVM_DTraceIsProbeEnabled(JNIEnv* env ATTR_UNUSED, jmethodID method ATTR_UNUSED) {
  LOG(FATAL) << "JVM_DTraceIsProbeEnabled is not implemented";
  return JNI_FALSE;
}

/*
 * Destroy custom DOF
 */
JNIEXPORT void JVM_DTraceDispose(JNIEnv* env ATTR_UNUSED, jlong activationHandle ATTR_UNUSED) {
  LOG(FATAL) << "JVM_DTraceDispose is not implemented";
}

/*
 * Check to see if DTrace is supported by OS
 */
JNIEXPORT jboolean JVM_DTraceIsSupported(JNIEnv* env ATTR_UNUSED) {
  LOG(FATAL) << "JVM_DTraceIsSupported is not implemented";
  return JNI_FALSE;
}

/*************************************************************************
 PART 2: Support for the Verifier and Class File Format Checker
 ************************************************************************/
/*
 * Return the class name in UTF format. The result is valid
 * until JVM_ReleaseUTf is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JVM_GetClassNameUTF(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetClassNameUTF is not implemented";
  return nullptr;
}

/*
 * Returns the constant pool types in the buffer provided by "types."
 */
JNIEXPORT void JVM_GetClassCPTypes(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                   unsigned char *types ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetClassCPTypes is not implemented";
}

/*
 * Returns the number of Constant Pool entries.
 */
JNIEXPORT jint JVM_GetClassCPEntriesCount(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetClassCPEntriesCount is not implemented";
  return 0;
}

/*
 * Returns the number of *declared* fields or methods.
 */
JNIEXPORT jint JVM_GetClassFieldsCount(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetClassFieldsCount is not implemented";
  return 0;
}

JNIEXPORT jint JVM_GetClassMethodsCount(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetClassMethodsCount is not implemented";
  return 0;
}

/*
 * Returns the CP indexes of exceptions raised by a given method.
 * Places the result in the given buffer.
 *
 * The method is identified by method_index.
 */
JNIEXPORT void JVM_GetMethodIxExceptionIndexes(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                               jint methodIndex ATTR_UNUSED,
                                               unsigned short *exceptions ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodIxExceptionIndexes is not implemented";
}
/*
 * Returns the number of exceptions raised by a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JVM_GetMethodIxExceptionsCount(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                              jint methodIndex ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodIxExceptionsCount is not implemented";
  return 0;
}

/*
 * Returns the byte code sequence of a given method.
 * Places the result in the given buffer.
 *
 * The method is identified by method_index.
 */
JNIEXPORT void JVM_GetMethodIxByteCode(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                       jint methodIndex ATTR_UNUSED, unsigned char *code ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodIxByteCode is not implemented";
}

/*
 * Returns the length of the byte code sequence of a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JVM_GetMethodIxByteCodeLength(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                             jint methodIndex ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodIxByteCodeLength is not implemented";
  return 0;
}

/*
 * A structure used to a capture exception table entry in a Java method.
 */
struct JVM_ExceptionTableEntryType {
  jint start_pc;
  jint end_pc;
  jint handler_pc;
  jint catchType;
};

/*
 * Returns the exception table entry at entry_index of a given method.
 * Places the result in the given buffer.
 *
 * The method is identified by method_index.
 */
JNIEXPORT void JVM_GetMethodIxExceptionTableEntry(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
    jint methodIndex ATTR_UNUSED, jint entryIndex ATTR_UNUSED,
    JVM_ExceptionTableEntryType *entry ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodIxExceptionTableEntry is not implemented";
}

/*
 * Returns the length of the exception table of a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JVM_GetMethodIxExceptionTableLength(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                                   int index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodIxExceptionTableLength is not implemented";
  return 0;
}

/*
 * Returns the modifiers of a given field.
 * The field is identified by field_index.
 */
JNIEXPORT jint JVM_GetFieldIxModifiers(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                       int index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetFieldIxModifiers is not implemented";
  return 0;
}

/*
 * Returns the modifiers of a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JVM_GetMethodIxModifiers(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                        int index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodIxModifiers is not implemented";
  return 0;
}
/*
 * Returns the number of local variables of a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JVM_GetMethodIxLocalsCount(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                          int index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodIxLocalsCount is not implemented";
  return 0;
}

/*
 * Returns the number of arguments (including this pointer) of a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JVM_GetMethodIxArgsSize(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                       int index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodIxArgsSize is not implemented";
  return 0;
}

/*
 * Returns the maximum amount of stack (in words) used by a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JVM_GetMethodIxMaxStack(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                       int index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodIxMaxStack is not implemented";
  return 0;
}

/*
 * Is a given method a constructor.
 * The method is identified by method_index.
 */
JNIEXPORT jboolean JVM_IsConstructorIx(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                       int index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_IsConstructorIx is not implemented";
  return JNI_FALSE;
}

/*
 * Is the given method generated by the VM.
 * The method is identified by method_index.
 */
JNIEXPORT jboolean JVM_IsVMGeneratedMethodIx(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                             int index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_IsVMGeneratedMethodIx is not implemented";
  return JNI_FALSE;
}

/*
 * Returns the name of a given method in UTF format.
 * The result remains valid until JVM_ReleaseUTF is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JVM_GetMethodIxNameUTF(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                              jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodIxNameUTF is not implemented";
  return nullptr;
}

/*
 * Returns the signature of a given method in UTF format.
 * The result remains valid until JVM_ReleaseUTF is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JVM_GetMethodIxSignatureUTF(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                                   jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetMethodIxSignatureUTF is not implemented";
  return nullptr;
}

/*
 * Returns the name of the field referred to at a given constant pool
 * index.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JVM_GetCPFieldNameUTF(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                             jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetCPFieldNameUTF is not implemented";
  return nullptr;
}

/*
 * Returns the name of the method referred to at a given constant pool
 * index.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JVM_GetCPMethodNameUTF(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                              jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetCPMethodNameUTF is not implemented";
  return nullptr;
}

/*
 * Returns the signature of the method referred to at a given constant pool
 * index.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JVM_GetCPMethodSignatureUTF(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                                   jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetCPMethodSignatureUTF is not implemented";
  return nullptr;
}

/*
 * Returns the signature of the field referred to at a given constant pool
 * index.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JVM_GetCPFieldSignatureUTF(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                                  jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetCPFieldSignatureUTF is not implemented";
  return nullptr;
}

/*
 * Returns the class name referred to at a given constant pool index.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JVM_GetCPClassNameUTF(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                             jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetCPClassNameUTF is not implemented";
  return nullptr;
}

/*
 * Returns the class name referred to at a given constant pool index.
 *
 * The constant pool entry must refer to a CONSTANT_Fieldref.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JVM_GetCPFieldClassNameUTF(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                                  jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetCPFieldClassNameUTF is not implemented";
  return nullptr;
}

/*
 * Returns the class name referred to at a given constant pool index.
 *
 * The constant pool entry must refer to CONSTANT_Methodref or
 * CONSTANT_InterfaceMethodref.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JVM_GetCPMethodClassNameUTF(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                                   jint index ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetCPMethodClassNameUTF is not implemented";
  return nullptr;
}

/*
 * Returns the modifiers of a field in calledClass. The field is
 * referred to in class cb at constant pool entry index.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 *
 * Returns -1 if the field does not exist in calledClass.
 */
JNIEXPORT jint JVM_GetCPFieldModifiers(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                       int index ATTR_UNUSED, jclass calledClass ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetCPFieldModifiers is not implemented";
  return 0;
}

/*
 * Returns the modifiers of a method in calledClass. The method is
 * referred to in class cb at constant pool entry index.
 *
 * Returns -1 if the method does not exist in calledClass.
 */
JNIEXPORT jint JVM_GetCPMethodModifiers(JNIEnv *env ATTR_UNUSED, jclass cb ATTR_UNUSED,
                                        int index ATTR_UNUSED, jclass calledClass ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetCPMethodModifiers is not implemented";
  return 0;
}

/*
 * Releases the UTF string obtained from the VM.
 */
JNIEXPORT void JVM_ReleaseUTF(const char *utf ATTR_UNUSED) {
  LOG(FATAL) << "JVM_ReleaseUTF is not implemented";
}

/*
 * Compare if two classes are in the same package.
 */
JNIEXPORT jboolean JVM_IsSameClassPackage(JNIEnv *env ATTR_UNUSED, jclass class1 ATTR_UNUSED,
                                          jclass class2 ATTR_UNUSED) {
  LOG(FATAL) << "JVM_IsSameClassPackage is not implemented";
  return JNI_FALSE;
}

/* Get classfile constants */
/*
 * A function defined by the byte-code verifier and called by the VM.
 * This is not a function implemented in the VM.
 *
 * Returns JNI_FALSE if verification fails. A detailed error message
 * will be places in msg_buf, whose length is specified by buf_len.
 */
using verifier_fn_t = jboolean (*)(JNIEnv *env, jclass cb, char * msgBuf, jint bufLen);

/*
 * Support for a VM-independent class format checker.
 */
struct MethodSizeInfo {
  unsigned long code;    /* byte code */
  unsigned long excs;    /* exceptions */
  unsigned long etab;    /* catch table */
  unsigned long lnum;    /* line number */
  unsigned long lvar;    /* local vars */
};

struct ClassSizeInfo {
  unsigned int constants;    /* constant pool */
  unsigned int fields;
  unsigned int methods;
  unsigned int interfaces;
  unsigned int fields2;      /* number of static 2-word fields */
  unsigned int innerclasses; /* # of records in InnerClasses attr */

  MethodSizeInfo clinit;     /* memory used in clinit */
  MethodSizeInfo main;       /* used everywhere else */
};

/*
 * Functions defined in libjava.so to perform string conversions.
 *
 */
using to_java_string_fn_t = jstring (*)(JNIEnv *env, char *str);

using to_c_string_fn_t = char *(*)(JNIEnv *env, jstring s, jboolean *b);

/* This is the function defined in libjava.so that performs class
 * format checks. This functions fills in size information about
 * the class file and returns:
 *
 *   0: good
 *  -1: out of memory
 *  -2: bad format
 *  -3: unsupported version
 *  -4: bad class name
 */
using check_format_fn_t = jint (*)(char *className,
    unsigned char *data,
    unsigned int dataSize,
    ClassSizeInfo *classSize,
    char *messageBuffer,
    jint bufferLength,
    jboolean measureOnly,
    jboolean checkRelaxed);

#define JVM_RECOGNIZED_CLASS_MODIFIERS (JVM_ACC_PUBLIC | \
    JVM_ACC_FINAL | \
    JVM_ACC_SUPER | \
    JVM_ACC_INTERFACE | \
    JVM_ACC_ABSTRACT | \
    JVM_ACC_ANNOTATION | \
    JVM_ACC_ENUM | \
    JVM_ACC_SYNTHETIC)

#define JVM_RECOGNIZED_FIELD_MODIFIERS (JVM_ACC_PUBLIC | \
    JVM_ACC_PRIVATE | \
    JVM_ACC_PROTECTED | \
    JVM_ACC_STATIC | \
    JVM_ACC_FINAL | \
    JVM_ACC_VOLATILE | \
    JVM_ACC_TRANSIENT | \
    JVM_ACC_ENUM | \
    JVM_ACC_SYNTHETIC)

#define JVM_RECOGNIZED_METHOD_MODIFIERS (JVM_ACC_PUBLIC | \
    JVM_ACC_PRIVATE | \
    JVM_ACC_PROTECTED | \
    JVM_ACC_STATIC | \
    JVM_ACC_FINAL | \
    JVM_ACC_SYNCHRONIZED | \
    JVM_ACC_BRIDGE | \
    JVM_ACC_VARARGS | \
    JVM_ACC_NATIVE | \
    JVM_ACC_ABSTRACT | \
    JVM_ACC_STRICT | \
    JVM_ACC_SYNTHETIC)

/*
 * This is the function defined in libjava.so to perform path
 * canonicalization. VM call this function before opening jar files
 * to load system classes.
 *
 */
using canonicalize_fn_t = int (*)(JNIEnv *env, char *orig, char *out, int len);

/*************************************************************************
 PART 3: I/O and Network Support
 ************************************************************************/
/* Note that the JVM IO functions are expected to return JVM_IO_ERR
 * when there is any kind of error. The caller can then use the
 * platform specific support (e.g., errno) to get the detailed
 * error info.  The JVM_GetLastErrorString procedure may also be used
 * to obtain a descriptive error string.
 */
#define JVM_IO_ERR  (-1)

/* For interruptible IO. Returning JVM_IO_INTR indicates that an IO
 * operation has been disrupted by Thread.interrupt. There are a
 * number of technical difficulties related to interruptible IO that
 * need to be solved. For example, most existing programs do not handle
 * InterruptedIOExceptions specially, they simply treat those as any
 * IOExceptions, which typically indicate fatal errors.
 *
 * There are also two modes of operation for interruptible IO. In the
 * resumption mode, an interrupted IO operation is guaranteed not to
 * have any side-effects, and can be restarted. In the termination mode,
 * an interrupted IO operation corrupts the underlying IO stream, so
 * that the only reasonable operation on an interrupted stream is to
 * close that stream. The resumption mode seems to be impossible to
 * implement on Win32 and Solaris. Implementing the termination mode is
 * easier, but it's not clear that's the right semantics.
 *
 * Interruptible IO is not supported on Win32.It can be enabled/disabled
 * using a compile-time flag on Solaris. Third-party JVM ports do not
 * need to implement interruptible IO.
 */
#define JVM_IO_INTR (-2)

/* Write a string into the given buffer, in the platform's local encoding,
 * that describes the most recent system-level error to occur in this thread.
 * Return the length of the string or zero if no error occurred.
 */
JNIEXPORT jint JVM_GetLastErrorString(char *buf, int len) {
#if defined(__GLIBC__) || defined(__BIONIC__)
  if (len == 0) {
    return 0;
  }
  const int err = errno;
  char* result = strerror_r(err, buf, len);
  if (result != buf) {
    (void)strncpy_s(buf, len, result, len);
    buf[len - 1] = '\0';
  }
  return strlen(buf);
#else
  UNUSED(buf);
  UNUSED(len);
  return -1;
#endif
}

/*
 * Convert a pathname into native format.  This function does syntactic
 * cleanup, such as removing redundant separator characters.  It modifies
 * the given pathname string in place.
 */
JNIEXPORT char * JVM_NativePath(char *path) {
  return path;
}

/*
 * JVM I/O error codes
 */
#define JVM_EEXIST       -100

/*
 * Open a file descriptor. This function returns a negative error code
 * on error, and a non-negative integer that is the file descriptor on
 * success.
 */
#define JVM_O_DELETE     0x10000

JNIEXPORT jint JVM_Open(const char *fname, jint flags, jint mode) {
  if (flags & JVM_O_DELETE) {
    LOG(FATAL) << "JVM_O_DELETE option is not supported (while opening: '" << fname << "')";
  }
  char canonical_path[PATH_MAX + 1] = {0x00};
  if ((strlen(fname) > PATH_MAX) || (realpath(fname, canonical_path) == nullptr)) {
    LOG(ERROR) << "Path " << fname << " does not exist.";
    return false;
  }
  int fd = TEMP_FAILURE_RETRY(open(canonical_path, flags & ~JVM_O_DELETE, mode));
  if (fd < 0) {
    int err = errno;
    if (err == EEXIST) {
      return JVM_EEXIST;
    } else {
      return -1;
    }
  }
  return fd;
}

/*
 * Close a file descriptor. This function returns -1 on error, and 0
 * on success.
 *
 * fd        the file descriptor to close.
 */
JNIEXPORT jint JVM_Close(jint fd) {
  return close(fd);
}

/*
 * Read data from a file decriptor into a char array.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer where to put the read data.
 * nbytes    the number of bytes to read.
 *
 * This function returns -1 on error, and 0 on success.
 */
JNIEXPORT jint JVM_Read(jint fd, char *buf, jint nBytes) {
  return TEMP_FAILURE_RETRY(read(fd, buf, nBytes));
}

/*
 * Write data from a char array to a file decriptor.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer from which to fetch the data.
 * nbytes    the number of bytes to write.
 *
 * This function returns -1 on error, and 0 on success.
 */
JNIEXPORT jint JVM_Write(jint fd, char *buf, jint nBytes) {
  return TEMP_FAILURE_RETRY(write(fd, buf, nBytes));
}

/*
 * Returns the number of bytes available for reading from a given file
 * descriptor
 */
JNIEXPORT jint JVM_Available(jint fd ATTR_UNUSED, jlong *pBytes ATTR_UNUSED) {
  LOG(FATAL) << "JVM_Available is not implemented";
  return 0;
}

/*
 * Move the file descriptor pointer from whence by offset.
 *
 * fd        the file descriptor to move.
 * offset    the number of bytes to move it by.
 * whence    the start from where to move it.
 *
 * This function returns the resulting pointer location.
 */
JNIEXPORT jlong JVM_Lseek(jint fd ATTR_UNUSED, jlong offset ATTR_UNUSED,
                          jint whence ATTR_UNUSED) {
  LOG(FATAL) << "JVM_Lseek is not implemented";
  return 0;
}

/*
 * Set the length of the file associated with the given descriptor to the given
 * length.  If the new length is longer than the current length then the file
 * is extended; the contents of the extended portion are not defined.  The
 * value of the file pointer is undefined after this procedure returns.
 */
JNIEXPORT jint JVM_SetLength(jint fd ATTR_UNUSED, jlong length ATTR_UNUSED) {
  LOG(FATAL) << "JVM_SetLength is not implemented";
  return 0;
}

/*
 * Synchronize the file descriptor's in memory state with that of the
 * physical device.  Return of -1 is an error, 0 is OK.
 */
JNIEXPORT jint JVM_Sync(jint fd) {
  return TEMP_FAILURE_RETRY(fsync(fd));
}

/*
 * Networking library support
 */
JNIEXPORT jint JVM_InitializeSocketLibrary(void) {
  return 0;
}

struct sockaddr;

JNIEXPORT jint JVM_Socket(jint domain, jint type, jint protocol) {
  return TEMP_FAILURE_RETRY(socket(domain, type, protocol));
}

JNIEXPORT jint JVM_SocketClose(jint fd) {
  return close(fd);
}

JNIEXPORT jint JVM_SocketShutdown(jint fd, jint howto) {
  return TEMP_FAILURE_RETRY(shutdown(fd, howto));
}

JNIEXPORT jint JVM_Recv(jint fd ATTR_UNUSED, char *buf ATTR_UNUSED,
                        jint nBytes ATTR_UNUSED, jint flags ATTR_UNUSED) {
  LOG(FATAL) << "JVM_Recv is not implemented";
  return 0;
}

JNIEXPORT jint JVM_Send(jint fd, char *buf, jint nBytes, jint flags) {
  return TEMP_FAILURE_RETRY(send(fd, buf, nBytes, flags));
}

JNIEXPORT jint JVM_Timeout(int fd ATTR_UNUSED, long timeout ATTR_UNUSED) {
  LOG(FATAL) << "JVM_Timeout is not implemented";
  return 0;
}

JNIEXPORT jint JVM_Listen(jint fd, jint count) {
  return TEMP_FAILURE_RETRY(listen(fd, count));
}

JNIEXPORT jint JVM_Connect(jint fd, struct sockaddr *him, jint len) {
  return TEMP_FAILURE_RETRY(connect(fd, him, len));
}

JNIEXPORT jint JVM_Bind(jint fd ATTR_UNUSED, struct sockaddr *him ATTR_UNUSED,
                        jint len ATTR_UNUSED) {
  LOG(FATAL) << "JVM_Bind is not implemented";
  return 0;
}

JNIEXPORT jint JVM_Accept(jint fd ATTR_UNUSED, struct sockaddr *him ATTR_UNUSED,
                          jint *len ATTR_UNUSED) {
  LOG(FATAL) << "JVM_Accept is not implemented";
  return 0;
}

JNIEXPORT jint JVM_RecvFrom(jint fd ATTR_UNUSED, char *buf ATTR_UNUSED, int nBytes ATTR_UNUSED,
    int flags ATTR_UNUSED, struct sockAddr *from ATTR_UNUSED, int *fromLen ATTR_UNUSED) {
  LOG(FATAL) << "JVM_RecvFrom is not implemented";
  return 0;
}

JNIEXPORT jint JVM_SendTo(jint fd ATTR_UNUSED, char *buf ATTR_UNUSED, int len ATTR_UNUSED,
    int flags ATTR_UNUSED, struct sockaddr *to ATTR_UNUSED, int toLen ATTR_UNUSED) {
  LOG(FATAL) << "JVM_SendTo is not implemented";
  return 0;
}

JNIEXPORT jint JVM_SocketAvailable(jint fd, jint *result) {
  if (TEMP_FAILURE_RETRY(ioctl(fd, FIONREAD, result)) < 0) {
    return JNI_FALSE;
  }
  return JNI_TRUE;
}

JNIEXPORT jint JVM_GetSockName(jint fd, struct sockaddr *addr, int *addrLen) {
  socklen_t len = *addrLen;
  int cc = TEMP_FAILURE_RETRY(getsockname(fd, addr, &len));
  *addrLen = len;
  return cc;
}

JNIEXPORT jint JVM_GetSockOpt(jint fd, int level, int optName, char *optVal, int *optLen) {
  socklen_t len = *optLen;
  int cc = TEMP_FAILURE_RETRY(getsockopt(fd, level, optName, optVal, &len));
  *optLen = len;
  return cc;
}

JNIEXPORT jint JVM_SetSockOpt(jint fd, int level, int optName, const char *optVal, int optLen) {
  return TEMP_FAILURE_RETRY(setsockopt(fd, level, optName, optVal, optLen));
}

JNIEXPORT int JVM_GetHostName(char* name, int nameLen) {
  return TEMP_FAILURE_RETRY(gethostname(name, nameLen));
}

/*
 * BE CAREFUL! The following functions do not implement the
 * full feature set of standard C printf formats.
 */
JNIEXPORT int jio_vsnprintf(char *str, size_t count, const char *fmt, va_list args) {
  if ((intptr_t)count <= 0) {
    return -1;
  }
  return vsnprintf_s(str, count, (count - 1), fmt, args);
}

JNIEXPORT int jio_snprintf(char *str, size_t count, const char *fmt, ...) {
  va_list args;
  int len;
  va_start(args, fmt);
  len = jio_vsnprintf(str, count, fmt, args);
  va_end(args);
  return len;
}

JNIEXPORT int jio_vfprintf(FILE *fp, const char *fmt, va_list args) {
  CHECK(fp != nullptr) << "fp is nullptr";
  return vfprintf(fp, fmt, args);
}

JNIEXPORT int jio_fprintf(FILE *fp, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int len = jio_vfprintf(fp, fmt, args);
  va_end(args);
  return len;
}

JNIEXPORT void *JVM_RawMonitorCreate(void) {
  pthread_mutex_t *mutex = reinterpret_cast<pthread_mutex_t*>(malloc(sizeof(pthread_mutex_t)));
  CHECK(mutex != nullptr);
  CHECK_PTHREAD_CALL(pthread_mutex_init, (mutex, nullptr), "JVM_RawMonitorCreate");
  return mutex;
}

JNIEXPORT void JVM_RawMonitorDestroy(void *mon) {
  CHECK_PTHREAD_CALL(pthread_mutex_destroy, (reinterpret_cast<pthread_mutex_t*>(mon)), "JVM_RawMonitorDestroy");
  free(mon);
}

JNIEXPORT jint JVM_RawMonitorEnter(void *mon) {
  return pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(mon));
}

JNIEXPORT void JVM_RawMonitorExit(void *mon) {
  CHECK_PTHREAD_CALL(pthread_mutex_unlock, (reinterpret_cast<pthread_mutex_t*>(mon)), "JVM_RawMonitorExit");
}

/*
 * java.lang.management support
 */
JNIEXPORT void *JVM_GetManagement(jint version ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetManagement is not implemented";
  return nullptr;
}

/*
 * com.sun.tools.attach.VirtualMachine support
 *
 * Initialize the agent properties with the properties maintained in the VM.
 */
JNIEXPORT jobject JVM_InitAgentProperties(JNIEnv *env ATTR_UNUSED, jobject agentProps ATTR_UNUSED) {
  LOG(FATAL) << "JVM_InitAgentProperties is not implemented";
  return nullptr;
}

JNIEXPORT jstring JVM_GetTemporaryDirectory(JNIEnv *env ATTR_UNUSED) {
  // no need to implement
  LOG(FATAL) << "JVM_GetTemporaryDirectory is not implemented";
  return nullptr;
}

/* Generics reflection support.
 *
 * Returns information about the given class's EnclosingMethod
 * attribute, if present, or null if the class had no enclosing
 * method.
 *
 * If non-null, the returned array contains three elements. Element 0
 * is the java.lang.Class of which the enclosing method is a member,
 * and elements 1 and 2 are the java.lang.Strings for the enclosing
 * method's name and descriptor, respectively.
 */
JNIEXPORT jobjectArray JVM_GetEnclosingMethodInfo(JNIEnv *env ATTR_UNUSED, jclass ofClass ATTR_UNUSED) {
  UNIMPLEMENTED(FATAL) << "JVM_GetEnclosingMethodInfo won't be called forever!";
  return nullptr;
}

/*
 * Java thread state support
 */
enum {
  JAVA_THREAD_STATE_NEW           = 0,
  JAVA_THREAD_STATE_RUNNABLE      = 1,
  JAVA_THREAD_STATE_BLOCKED       = 2,
  JAVA_THREAD_STATE_WAITING       = 3,
  JAVA_THREAD_STATE_TIMED_WAITING = 4,
  JAVA_THREAD_STATE_TERMINATED    = 5,
  JAVA_THREAD_STATE_COUNT         = 6
};

/*
 * Returns an array of the threadStatus values representing the
 * given Java thread state.  Returns NULL if the VM version is
 * incompatible with the JDK or doesn't support the given
 * Java thread state.
 */
JNIEXPORT jintArray JVM_GetThreadStateValues(JNIEnv *env ATTR_UNUSED, jint javaThreadState ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetThreadStateValues is not implemented";
  return nullptr;
}

/*
 * Returns an array of the substate names representing the
 * given Java thread state.  Returns NULL if the VM version is
 * incompatible with the JDK or the VM doesn't support
 * the given Java thread state.
 * values must be the jintArray returned from JVM_GetThreadStateValues
 * and javaThreadState.
 */
JNIEXPORT jobjectArray JVM_GetThreadStateNames(JNIEnv *env ATTR_UNUSED, jint javaThreadState ATTR_UNUSED,
                                               jintArray values ATTR_UNUSED) {
  LOG(FATAL) << "JVM_GetThreadStateNames is not implemented";
  return nullptr;
}

/*
 * Returns true if the JVM's lookup cache indicates that this class is
 * known to NOT exist for the given loader.
 */
JNIEXPORT jboolean JVM_KnownToNotExist(JNIEnv *env ATTR_UNUSED, jobject loader ATTR_UNUSED,
                                       const char *className ATTR_UNUSED) {
  // no need to implement
  LOG(FATAL) << "JVM_KnownToNotExist is not implemented";
  return JNI_FALSE;;
}

/*
 * Returns an array of all URLs that are stored in the JVM's lookup cache
 * for the given loader. NULL if the lookup cache is unavailable.
 */
JNIEXPORT jobjectArray JVM_GetResourceLookupCacheURLs(JNIEnv *env ATTR_UNUSED, jobject loader ATTR_UNUSED) {
  // no need to implement
  return nullptr;
}

/*
 * Returns an array of all URLs that *may* contain the resource_name for the
 * given loader. This function returns an integer array, each element
 * of which can be used to index into the array returned by
 * JVM_GetResourceLookupCacheURLs of the same loader to determine the
 * URLs.
 */
JNIEXPORT jintArray JVM_GetResourceLookupCache(JNIEnv *env ATTR_UNUSED, jobject loader ATTR_UNUSED,
                                               const char *resourceName ATTR_UNUSED) {
  // no need to implement
  LOG(FATAL) << "JVM_GetResourceLookupCache is not implemented";
  return nullptr;
}


/* =========================================================================
 * The following defines a private JVM interface that the JDK can query
 * for the JVM version and capabilities.  sun.misc.Version defines
 * the methods for getting the VM version and its capabilities.
 *
 * When a new bit is added, the following should be updated to provide
 * access to the new capability:
 *    HS:   JVM_GetVersionInfo and Abstract_VM_Version class
 *    SDK:  Version class
 *
 * Similary, a private JDK interface JDK_GetVersionInfo0 is defined for
 * JVM to query for the JDK version and capabilities.
 *
 * When a new bit is added, the following should be updated to provide
 * access to the new capability:
 *    HS:   JDK_Version class
 *    SDK:  JDK_GetVersionInfo0
 *
 * ==========================================================================
 */
struct jvm_version_info {
  /* Naming convention of RE build version string: n.n.n[_uu[c]][-<identifier>]-bxx */
  unsigned int jvm_version;   /* Consists of major, minor, micro (n.n.n) */
                              /* and build number (xx) */
  unsigned int update_version : 8;         /* Update release version (uu) */
  unsigned int special_update_version : 8; /* Special update release version (c) */
  unsigned int reserved1 : 16;
  unsigned int reserved2;

  /* The following bits represents JVM supports that JDK has dependency on.
   * JDK can use these bits to determine which JVM version
   * and support it has to maintain runtime compatibility.
   *
   * When a new bit is added in a minor or update release, make sure
   * the new bit is also added in the main/baseline.
   */
  unsigned int is_attach_supported : 1;
  unsigned int : 31;
  unsigned int : 32;
  unsigned int : 32;
};

#define JVM_VERSION_MAJOR(version) ((version & 0xFF000000) >> 24)
#define JVM_VERSION_MINOR(version) ((version & 0x00FF0000) >> 16)
#define JVM_VERSION_MICRO(version) ((version & 0x0000FF00) >> 8)

/* Build number is available only for RE builds.
 * It will be zero for internal builds.
 */
#define JVM_VERSION_BUILD(version) ((version & 0x000000FF))

JNIEXPORT void JVM_GetVersionInfo(JNIEnv *env ATTR_UNUSED, jvm_version_info *info, size_t infoSize) {
  errno_t E_num = memset_s(info, sizeof(*info), 0, infoSize);
  if (UNLIKELY(E_num != EOK)) {
    LOG(ERROR) << "memset_s(info, sizeof(*info), 0, infoSize) in JVM_GetVersionInfo return"
               << E_num << "rather than 0" << maple::endl;
  }
  // when we add a new capability in the jvm_version_info struct, we should also
  // consider to expose this new capability in the sun.rt.jvmCapabilities jvmstat
  // counter defined in runtimeService.cpp.
  info->jvm_version = ((0 & 0xFF) << 24) |
    ((0 & 0xFF) << 16) |
    (0 & 0xFF);
  info->update_version = 0;          // 0 in HotSpot Express VM
  info->special_update_version = 0;   // 0 in HotSpot Express VM
}

struct jdk_version_info {
  // Naming convention of RE build version string: n.n.n[_uu[c]][-<identifier>]-bxx
  unsigned int jdk_version;   /* Consists of major, minor, micro (n.n.n) */
                                /* and build number (xx) */
  unsigned int update_version : 8;         /* Update release version (uu) */
  unsigned int special_update_version : 8; /* Special update release version (c) */
  unsigned int reserved1 : 16;
  unsigned int reserved2;

    /* The following bits represents new JDK supports that VM has dependency on.
     * VM implementation can use these bits to determine which JDK version
     * and support it has to maintain runtime compatibility.
     *
     * When a new bit is added in a minor or update release, make sure
     * the new bit is also added in the main/baseline.
     */
  unsigned int thread_park_blocker : 1;
  unsigned int post_vm_init_hook_enabled : 1;
  unsigned int pending_list_uses_discovered_field : 1;
  unsigned int : 29;
  unsigned int : 32;
  unsigned int : 32;
};

#define JDK_VERSION_MAJOR(version) ((version & 0xFF000000) >> 24)
#define JDK_VERSION_MINOR(version) ((version & 0x00FF0000) >> 16)
#define JDK_VERSION_MICRO(version) ((version & 0x0000FF00) >> 8)

/* Build number is available only for RE build (i.e. JDK_BUILD_NUMBER is set to bNN)
 * It will be zero for internal builds.
 */
#define JDK_VERSION_BUILD(version) ((version & 0x000000FF))

/*
 * This is the function JDK_GetVersionInfo0 defined in libjava.so
 * that is dynamically looked up by JVM.
 */
using jdk_version_info_fn_t = void (*)(jdk_version_info* info, size_t infoSize);

/*
 * This structure is used by the launcher to get the default thread
 * stack size from the VM using JNI_GetDefaultJavaVMInitArgs() with a
 * version of 1.1.  As it is not supported otherwise, it has been removed
 * from jni.h
 */
struct JDK1_1InitArgs {
  jint version;

  char **properties;
  jint checkSource;
  jint nativeStackSize;
  jint javaStackSize;
  jint minHeapSize;
  jint maxHeapSize;
  jint verifyMode;
  char *classpath;

  jint (*vfprintf)(FILE *fp, const char *format, va_list args);
  void (*exit)(jint code);
  void (*abort)(void);

  jint enableClassGC;
  jint enableVerboseGC;
  jint disableAsyncGC;
  jint verbose;
  jboolean debugging;
  jint debugPort;
};

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

//#endif  !_JAVASOFT_JVM_H_ */
