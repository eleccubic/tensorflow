/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_COMPILER_TF2XLA_XLA_COMPILATION_DEVICE_H_
#define TENSORFLOW_COMPILER_TF2XLA_XLA_COMPILATION_DEVICE_H_

#include <memory>

#include "tensorflow/compiler/xla/xla_data.pb.h"
#include "tensorflow/core/common_runtime/local_device.h"
#include "tensorflow/core/framework/device_base.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/mem.h"
#include "tensorflow/core/public/session_options.h"

namespace tensorflow {

// Class is defined in xla_compilation_device.cc, reference
// included here only so the XlaCompilationDevice allocator_ member can be
// declared.
class XlaCompilationAllocator;

// Deliberately don't register the device factory because we *never*
// want soft placement to put Ops on an JIT device. Tests can include
// the tla_jit_test_deps target which registers the factory, and when
// using JIT in practice, the device is created manually not using a
// factory.

// This is a 'dummy' TensorFlow device that is only used to execute a
// subgraph of XLA compilation Ops to construct a compiled version
// of the subgraph's computation. It has a 'dummy' allocator that
// backs each Tensor with metadata indicating the computation the
// Tensor represents.
class XlaCompilationDevice : public LocalDevice {
 public:
  XlaCompilationDevice(const SessionOptions& options, DeviceType type);

  ~XlaCompilationDevice() override;

  Allocator* GetAllocator(AllocatorAttributes attr) override;

  Status Sync() override;

  Status MakeTensorFromProto(const TensorProto& tensor_proto,
                             const AllocatorAttributes alloc_attrs,
                             Tensor* tensor) override;

 private:
  std::unique_ptr<XlaCompilationAllocator> allocator_;
};

// A XlaExpression wraps an XLA computation. Each Tensor on an
// XlaCompilationDevice contains an XlaExpression, and the shape of the Tensor
// matches the shape of the subcomputation in the ComputationDataHandle. Each
// expression is either a constant, or a function of previously-compiled
// expressions.
class XlaExpression {
 public:
  XlaExpression();

  // handle() stores the XLA handle of the computation that the
  // expression represents.
  void set_handle(const xla::ComputationDataHandle& h);
  const xla::ComputationDataHandle& handle() const { return handle_; }

  void set_constant_value(Tensor value);
  bool has_constant_value() const { return has_constant_value_; }
  const Tensor& constant_value() const { return constant_value_; }

  void set_variable_id(int id);
  int variable_id() const { return variable_id_; }

 private:
  // The XLA handle of the expression's computation.
  xla::ComputationDataHandle handle_;

  // If this expression is a constant with a known value, 'constant_value' is a
  // host-memory Tensor containing the value. Used to avoid invoking XLA for
  // expressions that are trivially constant.
  bool has_constant_value_ = false;
  Tensor constant_value_;

  int variable_id_ = -1;

  TF_DISALLOW_COPY_AND_ASSIGN(XlaExpression);
};

}  // namespace tensorflow

#endif  // TENSORFLOW_COMPILER_TF2XLA_XLA_COMPILATION_DEVICE_H_
