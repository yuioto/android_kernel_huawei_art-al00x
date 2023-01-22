/* Copyright (C) 2014 The Android Open Source Project
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This file implements interfaces from the file jvm.h. This implementation
 * is licensed under the same terms as the file jvm.h.  The
 * copyright and license information for the file jvm.h follows.
 *
 * Copyright (c) 1997, 2011, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

/*
 * Services that OpenJDK expects the VM to provide.
 */
#include <dlfcn.h>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <algorithm>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "jvm.h"
#include "ScopedUtfChars.h"
#include "base/logging.h"
#include "java_vm_ext.h"
#include "jni_env_ext.h"
#include "monitor.h"
#include "runtime.h"

#undef LOG_TAG
#define LOG_TAG "mplopenjdk"

using namespace std;

const static unsigned int PATH_MAX_LENGTH = 4096;

/* posix open() with extensions; used by e.g. ZipFile */
JNIEXPORT jint JVM_Open(const char *fname, jint flags, jint mode) {
  /*
   * Some code seems to want the special return value JVM_EEXIST if the
   * file open fails due to O_EXCL.
   */
  // Don't use JVM_O_DELETE, it's problematic with FUSE, see b/28901232.
  if (flags & JVM_O_DELETE) {
    LOG(FATAL) << "JVM_O_DELETE option is not supported (while opening: '" << fname << "')";
  }

  char canonical_path[PATH_MAX_LENGTH + 1] = { 0x00 };
  if ((strlen(fname) > PATH_MAX_LENGTH) || (nullptr == realpath(fname, canonical_path))) {
    LOG(ERROR) << "Path " << fname << " does not exist.";
    return -1;
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

/* posix close() */
JNIEXPORT jint JVM_Close(jint fd) {
  // don't want TEMP_FAILURE_RETRY here -- file is closed even if EINTR
  return close(fd);
}

/* posix read() */
JNIEXPORT jint JVM_Read(jint fd, char *buf, jint nbytes) {
  return TEMP_FAILURE_RETRY(read(fd, buf, nbytes));
}

/* posix write(); is used to write messages to stderr */
JNIEXPORT jint JVM_Write(jint fd, char *buf, jint nbytes) {
  return TEMP_FAILURE_RETRY(write(fd, buf, nbytes));
}

/* posix lseek() */
JNIEXPORT jlong JVM_Lseek(jint fd, jlong offset, jint whence) {
#if !defined(__APPLE__)
  // NOTE: Using TEMP_FAILURE_RETRY here is busted for LP32 on glibc - the return
  // value will be coerced into an int32_t.
  //
  // lseek64 isn't specified to return EINTR so it shouldn't be necessary
  // anyway.
  return lseek64(fd, offset, whence);
#else
  // NOTE: This code is compiled for Mac OS but isn't ever run on that
  // platform.
  return lseek(fd, offset, whence);
#endif
}

/*
 * "raw monitors" seem to be expected to behave like non-recursive pthread
 * mutexes.  They're used by ZipFile.
 */
JNIEXPORT void *JVM_RawMonitorCreate(void) {
  pthread_mutex_t *mutex = reinterpret_cast<pthread_mutex_t *>(malloc(sizeof(pthread_mutex_t)));
  CHECK(mutex != nullptr);
  CHECK_PTHREAD_CALL(pthread_mutex_init, (mutex, nullptr), "JVM_RawMonitorCreate");
  return mutex;
}

JNIEXPORT void JVM_RawMonitorDestroy(void *mon) {
  CHECK_PTHREAD_CALL(pthread_mutex_destroy, (reinterpret_cast<pthread_mutex_t *>(mon)), "JVM_RawMonitorDestroy");
  free(mon);
}

JNIEXPORT jint JVM_RawMonitorEnter(void *mon) {
  return pthread_mutex_lock(reinterpret_cast<pthread_mutex_t *>(mon));
}

JNIEXPORT void JVM_RawMonitorExit(void *mon) {
  CHECK_PTHREAD_CALL(pthread_mutex_unlock, (reinterpret_cast<pthread_mutex_t *>(mon)), "JVM_RawMonitorExit");
}

JNIEXPORT char *JVM_NativePath(char *path) {
  return path;
}

JNIEXPORT jint JVM_GetLastErrorString(char *buf, int len) {
#if defined(__GLIBC__) || defined(__BIONIC__)

  if (len == 0) {
    return 0;
  }

  const int err = errno;
  char *result = strerror_r(err, buf, len);

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

JNIEXPORT int jio_fprintf(FILE *fp, const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  int len = jio_vfprintf(fp, fmt, args);
  va_end(args);

  return len;
}

JNIEXPORT int jio_vfprintf(FILE *fp, const char *fmt, va_list args) {
  CHECK(fp != NULL);
  return vfprintf(fp, fmt, args);
}

/* posix fsync() */
JNIEXPORT jint JVM_Sync(jint fd) {
  return TEMP_FAILURE_RETRY(fsync(fd));
}

JNIEXPORT void *JVM_FindLibraryEntry(void *handle, const char *name) {
  return dlsym(handle, name);
}

JNIEXPORT jlong JVM_CurrentTimeMillis(JNIEnv *env ATTR_UNUSED, jclass clazz ATTR_UNUSED) {
  struct timeval tv;
  (void)gettimeofday(&tv, (struct timezone *)NULL);
  jlong when = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
  return when;
}

JNIEXPORT jint JVM_Socket(jint domain, jint type, jint protocol) {
  return TEMP_FAILURE_RETRY(socket(domain, type, protocol));
}

JNIEXPORT jint JVM_InitializeSocketLibrary() {
  return 0;
}

int jio_vsnprintf(char *str, size_t count, const char *fmt, va_list args) {
  if ((intptr_t)count <= 0) {
    return -1;
  }

  return vsnprintf_s(str, count, (count - 1), fmt, args);
}

int jio_snprintf(char *str, size_t count, const char *fmt, ...) {
  va_list args;
  int len;
  va_start(args, fmt);
  len = jio_vsnprintf(str, count, fmt, args);
  va_end(args);
  return len;
}

JNIEXPORT jint JVM_SetSockOpt(jint fd, int level, int optName, const char *optval, int optlen) {
  return TEMP_FAILURE_RETRY(setsockopt(fd, level, optName, optval, optlen));
}

JNIEXPORT jint JVM_SocketShutdown(jint fd, jint howto) {
  return TEMP_FAILURE_RETRY(shutdown(fd, howto));
}

JNIEXPORT jint JVM_GetSockOpt(jint fd, int level, int optName, char *optval, int *optlen) {
  socklen_t len = *optlen;
  int cc = TEMP_FAILURE_RETRY(getsockopt(fd, level, optName, optval, &len));
  *optlen = len;
  return cc;
}

JNIEXPORT jint JVM_GetSockName(jint fd, struct sockaddr *addr, int *addrlen) {
  socklen_t len = *addrlen;
  int cc = TEMP_FAILURE_RETRY(getsockname(fd, addr, &len));
  *addrlen = len;
  return cc;
}

JNIEXPORT jint JVM_SocketAvailable(jint fd, jint *result) {
  if (TEMP_FAILURE_RETRY(ioctl(fd, FIONREAD, result)) < 0) {
    return JNI_FALSE;
  }

  return JNI_TRUE;
}

JNIEXPORT jint JVM_Send(jint fd, char *buf, jint nBytes, jint flags) {
  return TEMP_FAILURE_RETRY(send(fd, buf, nBytes, flags));
}

JNIEXPORT jint JVM_SocketClose(jint fd) {
  // Don't want TEMP_FAILURE_RETRY here -- file is closed even if EINTR.
  return close(fd);
}

JNIEXPORT jint JVM_Listen(jint fd, jint count) {
  return TEMP_FAILURE_RETRY(listen(fd, count));
}

JNIEXPORT jint JVM_Connect(jint fd, struct sockaddr *addr, jint addrlen) {
  return TEMP_FAILURE_RETRY(connect(fd, addr, addrlen));
}

JNIEXPORT int JVM_GetHostName(char *name, int namelen) {
  return TEMP_FAILURE_RETRY(gethostname(name, namelen));
}

JNIEXPORT jstring JVM_InternString(JNIEnv *env ATTR_UNUSED, jstring jstr) {
  UNIMPLEMENTED(FATAL) << "JVM_InternString should not be invoked";
  // only used in the JNI native of java/lang/String which
  // invokes Java_java_lang_String_intern__  instead in maplert
  return jstr;
}

JNIEXPORT jlong JVM_FreeMemory(void) {
  // returns the free memory within the limit
  return (jlong)maple::Runtime::Current()->GetGc()->GetFreeMemory();
}

JNIEXPORT jlong JVM_TotalMemory(void) {
  // returns the approximate current memory consumed including internal overhead
  return (jlong)maple::Runtime::Current()->GetGc()->GetTotalMemory();
}

JNIEXPORT jlong JVM_MaxMemory(void) {
  return (jlong)maple::Runtime::Current()->GetGc()->GetMaxMemory();
}

JNIEXPORT void JVM_GC(void) {
  maple::Runtime::Current()->GetGc()->TriggerGC(maplert::kGCReasonUser);
}

JNIEXPORT __attribute__((noreturn)) void JVM_Exit(jint status) {
  LOG(INFO) << "System.exit called, status: " << status;
  maple::Runtime::Current()->CallExitHook(status);
  exit(status);
}

JNIEXPORT jstring JVM_NativeLoad(JNIEnv *env, jstring javaFilename, jobject javaLoader, jstring javaLibrarySearchPath) {
  ScopedUtfChars filename(env, javaFilename);

  if (filename.c_str() == NULL) {
    return NULL;
  }

  std::string errorMsg;
  {
    maple::JavaVMExt *vm = maple::Runtime::Current()->GetJavaVM();
    std::string path = filename.c_str();
    void *handle = vm->LoadNativeLibrary(env, path, javaLoader, javaLibrarySearchPath, &errorMsg);

    if (handle != nullptr) {
      return nullptr;
    }
  }

  // Don't let a pending exception from JNI_OnLoad cause a CheckJNI issue with NewStringUTF.
  env->ExceptionClear();
  return env->NewStringUTF(errorMsg.c_str());
}

JNIEXPORT jstring JVM_NativeLoad(JNIEnv *env, jstring javaFilename, jobject javaLoader,
                                 jclass caller ATTR_UNUSED) {
  // Parameter caller can not be provided in maple, then it is setted ATTR_UNUSED

  ScopedUtfChars filename(env, javaFilename);

  if (filename.c_str() == NULL) {
    return NULL;
  }

  std::string errorMsg;
  {
    maple::JavaVMExt *vm = maple::Runtime::Current()->GetJavaVM();
    void *handle = vm->LoadNativeLibrary(env, filename.c_str(), javaLoader, nullptr, &errorMsg);

    if (handle != nullptr) {
      return nullptr;
    }
  }

  // Don't let a pending exception from JNI_OnLoad cause a CheckJNI issue with NewStringUTF.
  env->ExceptionClear();
  return env->NewStringUTF(errorMsg.c_str());
}

JNIEXPORT void JVM_StartThread(JNIEnv *, jobject, jlong, jboolean) {
  // see libcore/ojluni/src/main/native/Thread.c, in art and maple runtime
  // we use vendor/open_source/platform-rt/shared/core/native/java_lang_Thread.cc
  UNIMPLEMENTED(FATAL) << "JVM_StartThread won't be called forever!";
}

JNIEXPORT void JVM_SetThreadPriority(JNIEnv *, jobject, jint) {
  // see libcore/ojluni/src/main/native/Thread.c, in art and maple runtime
  // we use vendor/open_source/platform-rt/shared/core/native/java_lang_Thread.cc
  UNIMPLEMENTED(FATAL) << "JVM_SetThreadPriority won't be called forever!";
}

JNIEXPORT void JVM_Yield(JNIEnv *env ATTR_UNUSED, jclass threadClass ATTR_UNUSED) {
  CHECK(sched_yield() == 0) << "sched_yield() return nonzero result" << endl;  // wait for a while
}

JNIEXPORT void JVM_Sleep(JNIEnv *, jclass, jobject, jlong) {
  // see libcore/ojluni/src/main/native/Thread.c, in art and maple runtime
  // we use vendor/open_source/platform-rt/shared/core/native/java_lang_Thread.cc
  UNIMPLEMENTED(FATAL) << "JVM_Sleep won't be called forever!";
}

JNIEXPORT jobject JVM_CurrentThread(JNIEnv *, jclass) {
  // see libcore/ojluni/src/main/native/Thread.c, in art and maple runtime
  // we use vendor/open_source/platform-rt/shared/core/native/java_lang_Thread.cc
  UNIMPLEMENTED(FATAL) << "JVM_CurrentThread won't be called forever!";
  return nullptr;
}

JNIEXPORT void JVM_Interrupt(JNIEnv *, jobject) {
  // see libcore/ojluni/src/main/native/Thread.c, in art and maple runtime
  // we use vendor/open_source/platform-rt/shared/core/native/java_lang_Thread.cc
  UNIMPLEMENTED(FATAL) << "JVM_Interrupt won't be called forever!";
}

JNIEXPORT jboolean JVM_IsInterrupted(JNIEnv *, jobject, jboolean) {
  // see libcore/ojluni/src/main/native/Thread.c, in art and maple runtime
  // we use vendor/open_source/platform-rt/shared/core/native/java_lang_Thread.cc
  UNIMPLEMENTED(FATAL) << "JVM_IsInterrupted won't be called forever!";
  return JNI_FALSE;
}

JNIEXPORT jboolean JVM_HoldsLock(JNIEnv *, jclass, jobject) {
  // see libcore/ojluni/src/main/native/Thread.c, in art and maple runtime
  // we use vendor/open_source/platform-rt/shared/core/native/java_lang_Thread.cc
  UNIMPLEMENTED(FATAL) << "JVM_HoldsLock won't be called forever!";
  return JNI_FALSE;
}

JNIEXPORT void JVM_SetNativeThreadName(JNIEnv *, jobject, jstring) {
  // see libcore/ojluni/src/main/native/Thread.c, in art and maple runtime
  // we use vendor/open_source/platform-rt/shared/core/native/java_lang_Thread.cc
  UNIMPLEMENTED(FATAL) << "JVM_SetNativeThreadName won't be called forever!";
}

JNIEXPORT jint JVM_IHashCode(JNIEnv *, jobject) {
  // in art, the function in not implemented
  UNIMPLEMENTED(FATAL) << "JVM_IHashCode is not implemented";
  return 0;
}

JNIEXPORT jlong JVM_NanoTime(JNIEnv *, jclass) {
  // in art, the function in not implemented
  UNIMPLEMENTED(FATAL) << "JVM_NanoTime is not implemented";
  return 0L;
}

JNIEXPORT void JVM_ArrayCopy(JNIEnv* /* env */, jclass /* unused */, jobject /* javaSrc */, jint /* srcPos */,
                             jobject /* javaDst */, jint /* dstPos */, jint /* length */) {
  // in art, the function in not implemented
  UNIMPLEMENTED(FATAL) << "JVM_ArrayCopy is not implemented";
}

JNIEXPORT jint JVM_FindSignal(const char *name ATTR_UNUSED) {
  // in art, the function in not implemented
  LOG(FATAL) << "JVM_FindSignal is not implemented";
  return 0;
}

JNIEXPORT void *JVM_RegisterSignal(jint signum ATTR_UNUSED, void *handler ATTR_UNUSED) {
  // in art, the function in not implemented
  LOG(FATAL) << "JVM_RegisterSignal is not implemented";
  return nullptr;
}

JNIEXPORT jboolean JVM_RaiseSignal(jint signum ATTR_UNUSED) {
  // in art, the function in not implemented
  LOG(FATAL) << "JVM_RaiseSignal is not implemented";
  return JNI_FALSE;
}

JNIEXPORT __attribute__((noreturn)) void JVM_Halt(jint code) {
  exit(code);
}

JNIEXPORT jboolean JVM_IsNaN(jdouble d) {
  return isnan(d);
}
