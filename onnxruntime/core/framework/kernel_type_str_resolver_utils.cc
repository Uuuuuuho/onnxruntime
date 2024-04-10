// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#if !defined(ORT_MINIMAL_BUILD) || defined(ORT_EXTENDED_MINIMAL_BUILD)

#include "core/framework/kernel_type_str_resolver_utils.h"

#include "core/common/flatbuffers.h"

#include "core/common/common.h"
#include "core/flatbuffers/schema/ort.fbs.h"
#include "core/optimizer/layout_transformation/layout_transformation_potentially_added_ops.h"

namespace onnxruntime::kernel_type_str_resolver_utils {

static constexpr auto* kStandaloneKernelTypeStrResolverFileIdentifier = "ktsr";

#if !defined(ORT_MINIMAL_BUILD)

gsl::span<const OpIdentifierWithStringViews> GetLayoutTransformationRequiredOpIdentifiers() {
  return kLayoutTransformationPotentiallyAddedOps;
}

Status SaveKernelTypeStrResolverToBuffer(const KernelTypeStrResolver& kernel_type_str_resolver,
                                         flatbuffers::DetachedBuffer& buffer, gsl::span<const uint8_t>& buffer_span) {
  flatbuffers::FlatBufferBuilder builder;
  flatbuffers::Offset<fbs::KernelTypeStrResolver> fbs_kernel_type_str_resolver;
  ORT_RETURN_IF_ERROR(kernel_type_str_resolver.SaveToOrtFormat(builder, fbs_kernel_type_str_resolver));
  builder.Finish(fbs_kernel_type_str_resolver, kStandaloneKernelTypeStrResolverFileIdentifier);
  buffer = builder.Release();
  buffer_span = gsl::make_span(buffer.data(), buffer.size());
  return Status::OK();
}

#endif  // !defined(ORT_MINIMAL_BUILD)

Status LoadKernelTypeStrResolverFromBuffer(KernelTypeStrResolver& kernel_type_str_resolver,
                                           gsl::span<const uint8_t> buffer_span) {
  flatbuffers::Verifier verifier{buffer_span.data(), buffer_span.size_bytes()};
  ORT_RETURN_IF_NOT(verifier.VerifyBuffer<fbs::KernelTypeStrResolver>(kStandaloneKernelTypeStrResolverFileIdentifier),
                    "Failed to verify KernelTypeStrResolver flatbuffers data.");
  const auto* fbs_kernel_type_str_resolver = flatbuffers::GetRoot<fbs::KernelTypeStrResolver>(buffer_span.data());
  ORT_RETURN_IF_ERROR(kernel_type_str_resolver.LoadFromOrtFormat(*fbs_kernel_type_str_resolver));
  return Status::OK();
}

Status AddLayoutTransformationRequiredOpsToKernelTypeStrResolver(KernelTypeStrResolver& kernel_type_str_resolver) {
  KernelTypeStrResolver resolver_with_required_ops{};

  // to generate kLayoutTransformationRequiredOpsKernelTypeStrResolverBytes, run the test:
  //   KernelTypeStrResolverUtilsTest.DISABLED_PrintExpectedLayoutTransformationRequiredOpsResolverByteArray

  // clang-format off
  constexpr uint8_t kLayoutTransformationRequiredOpsKernelTypeStrResolverBytes[] = {
      0x10, 0x00, 0x00, 0x00, 0x6b, 0x74, 0x73, 0x72, 0x00, 0x00, 0x06, 0x00, 0x08, 0x00, 0x04, 0x00,
      0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x5c, 0x02, 0x00, 0x00,
      0xdc, 0x00, 0x00, 0x00, 0x6c, 0x01, 0x00, 0x00, 0x68, 0x07, 0x00, 0x00, 0xd4, 0x09, 0x00, 0x00,
      0x1c, 0x09, 0x00, 0x00, 0xcc, 0x04, 0x00, 0x00, 0xc8, 0x0c, 0x00, 0x00, 0x7c, 0x06, 0x00, 0x00,
      0x34, 0x0a, 0x00, 0x00, 0x70, 0x09, 0x00, 0x00, 0x24, 0x06, 0x00, 0x00, 0xfc, 0x04, 0x00, 0x00,
      0xf4, 0x07, 0x00, 0x00, 0x38, 0x0b, 0x00, 0x00, 0xbc, 0x02, 0x00, 0x00, 0x7c, 0x05, 0x00, 0x00,
      0x34, 0x00, 0x00, 0x00, 0xac, 0x01, 0x00, 0x00, 0x90, 0x07, 0x00, 0x00, 0x5c, 0x0a, 0x00, 0x00,
      0xb0, 0x05, 0x00, 0x00, 0xd8, 0x0c, 0x00, 0x00, 0x3c, 0x04, 0x00, 0x00, 0x98, 0x0a, 0x00, 0x00,
      0x60, 0x08, 0x00, 0x00, 0x80, 0x06, 0x00, 0x00, 0x80, 0x0b, 0x00, 0x00, 0xd0, 0x02, 0x00, 0x00,
      0xd0, 0x0b, 0x00, 0x00, 0x20, 0xf3, 0xff, 0xff, 0x14, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
      0x3a, 0x53, 0x71, 0x75, 0x65, 0x65, 0x7a, 0x65, 0x3a, 0x31, 0x33, 0x00, 0x48, 0xf3, 0xff, 0xff,
      0x8c, 0x0a, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0xe4, 0xf3, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x64, 0xf3, 0xff, 0xff, 0xac, 0x0c, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x52, 0xf3, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x4c, 0xf3, 0xff, 0xff, 0x88, 0xf3, 0xff, 0xff,
      0x18, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00,
      0x64, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x3a, 0x44, 0x65, 0x71,
      0x75, 0x61, 0x6e, 0x74, 0x69, 0x7a, 0x65, 0x4c, 0x69, 0x6e, 0x65, 0x61, 0x72, 0x3a, 0x31, 0x33,
      0x00, 0x00, 0x00, 0x00, 0xc0, 0xf3, 0xff, 0xff, 0x84, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xaa, 0xf3, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01,
      0xdc, 0xf3, 0xff, 0xff, 0x34, 0x0c, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x7c, 0xf4, 0xff, 0xff, 0x02, 0x00, 0x00, 0x00,
      0xc4, 0xf3, 0xff, 0xff, 0x00, 0xf4, 0xff, 0xff, 0xcc, 0x02, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x9c, 0xf4, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00,
      0x1c, 0xf4, 0xff, 0xff, 0x14, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      0x4c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x3a, 0x44, 0x65, 0x71,
      0x75, 0x61, 0x6e, 0x74, 0x69, 0x7a, 0x65, 0x4c, 0x69, 0x6e, 0x65, 0x61, 0x72, 0x3a, 0x31, 0x39,
      0x00, 0x00, 0x00, 0x00, 0x50, 0xf4, 0xff, 0xff, 0xe0, 0x0a, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3e, 0xf4, 0xff, 0xff,
      0x00, 0x00, 0x00, 0x01, 0xf8, 0xf4, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x78, 0xf4, 0xff, 0xff,
      0xe8, 0x0a, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x18, 0xf5, 0xff, 0xff, 0x02, 0x00, 0x00, 0x00, 0x60, 0xf4, 0xff, 0xff,
      0x9c, 0xf4, 0xff, 0xff, 0x14, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      0x18, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x3a, 0x53, 0x71, 0x75,
      0x65, 0x65, 0x7a, 0x65, 0x3a, 0x32, 0x31, 0x00, 0xc4, 0xf4, 0xff, 0xff, 0x4c, 0x0b, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0xb2, 0xf4, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0xac, 0xf4, 0xff, 0xff, 0xe8, 0xf4, 0xff, 0xff,
      0xec, 0x08, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x84, 0xf5, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x04, 0xf5, 0xff, 0xff, 0x18, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00,
      0x20, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x3a, 0x44, 0x65, 0x71, 0x75, 0x61, 0x6e, 0x74,
      0x69, 0x7a, 0x65, 0x4c, 0x69, 0x6e, 0x65, 0x61, 0x72, 0x3a, 0x31, 0x30, 0x00, 0x00, 0x00, 0x00,
      0x3c, 0xf5, 0xff, 0xff, 0x08, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x79, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x2e, 0xf5, 0xff, 0xff,
      0x00, 0x00, 0x00, 0x01, 0x60, 0xf5, 0xff, 0xff, 0xb0, 0x0a, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0xf6, 0xff, 0xff,
      0x02, 0x00, 0x00, 0x00, 0x48, 0xf5, 0xff, 0xff, 0x84, 0xf5, 0xff, 0xff, 0x48, 0x01, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x20, 0xf6, 0xff, 0xff,
      0x01, 0x00, 0x00, 0x00, 0xa0, 0xf5, 0xff, 0xff, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x3a, 0x53, 0x71, 0x75,
      0x65, 0x65, 0x7a, 0x65, 0x3a, 0x31, 0x00, 0x00, 0xc4, 0xf5, 0xff, 0xff, 0x4c, 0x0a, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0xb2, 0xf5, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0xac, 0xf5, 0xff, 0xff, 0xe8, 0xf5, 0xff, 0xff,
      0x28, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00,
      0x78, 0x00, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x94, 0x00, 0x00, 0x00,
      0xb8, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x63, 0x6f, 0x6d, 0x2e,
      0x6d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x3a, 0x51, 0x4c, 0x69, 0x6e, 0x65, 0x61,
      0x72, 0x43, 0x6f, 0x6e, 0x76, 0x3a, 0x31, 0x00, 0x34, 0xf6, 0xff, 0xff, 0x18, 0x05, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xd0, 0xf6, 0xff, 0xff,
      0x06, 0x00, 0x00, 0x00, 0x50, 0xf6, 0xff, 0xff, 0x08, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x54, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0xf4, 0xf6, 0xff, 0xff, 0x08, 0x00, 0x00, 0x00, 0x74, 0xf6, 0xff, 0xff, 0xbc, 0x08, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x14, 0xf7, 0xff, 0xff, 0x05, 0x00, 0x00, 0x00, 0x1c, 0xf7, 0xff, 0xff, 0x03, 0x00, 0x00, 0x00,
      0x9c, 0xf6, 0xff, 0xff, 0x08, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
      0x77, 0x5f, 0x73, 0x63, 0x61, 0x6c, 0x65, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x44, 0xf7, 0xff, 0xff, 0x04, 0x00, 0x00, 0x00, 0xc4, 0xf6, 0xff, 0xff, 0x08, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x78, 0x5f, 0x73, 0x63, 0x61, 0x6c, 0x65, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6c, 0xf7, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00,
      0xec, 0xf6, 0xff, 0xff, 0x74, 0x08, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x8c, 0xf7, 0xff, 0xff, 0x02, 0x00, 0x00, 0x00,
      0xd4, 0xf6, 0xff, 0xff, 0x10, 0xf7, 0xff, 0xff, 0x08, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x54, 0x33, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x06, 0xf7, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0xc0, 0xf7, 0xff, 0xff,
      0x07, 0x00, 0x00, 0x00, 0x40, 0xf7, 0xff, 0xff, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x3a, 0x55, 0x6e, 0x73,
      0x71, 0x75, 0x65, 0x65, 0x7a, 0x65, 0x3a, 0x31, 0x31, 0x00, 0x00, 0x00, 0x68, 0xf7, 0xff, 0xff,
      0xa8, 0x08, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x56, 0xf7, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x50, 0xf7, 0xff, 0xff,
      0x8c, 0xf7, 0xff, 0xff, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x14, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x3a, 0x49, 0x64, 0x65, 0x6e, 0x74, 0x69, 0x74,
      0x79, 0x3a, 0x31, 0x00, 0xb0, 0xf7, 0xff, 0xff, 0x60, 0x08, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x9e, 0xf7, 0xff, 0xff,
      0x00, 0x00, 0x00, 0x01, 0x98, 0xf7, 0xff, 0xff, 0xd4, 0xf7, 0xff, 0xff, 0x18, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
      0x5c, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x3a, 0x51, 0x75, 0x61, 0x6e, 0x74, 0x69, 0x7a,
      0x65, 0x4c, 0x69, 0x6e, 0x65, 0x61, 0x72, 0x3a, 0x31, 0x30, 0x00, 0x00, 0x08, 0xf8, 0xff, 0xff,
      0x28, 0x07, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0xf6, 0xf7, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0xb0, 0xf8, 0xff, 0xff,
      0x02, 0x00, 0x00, 0x00, 0x30, 0xf8, 0xff, 0xff, 0x30, 0x07, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0c, 0xf8, 0xff, 0xff, 0x48, 0xf8, 0xff, 0xff,
      0x04, 0x03, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0xe4, 0xf8, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x64, 0xf8, 0xff, 0xff, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
      0x3a, 0x53, 0x71, 0x75, 0x65, 0x65, 0x7a, 0x65, 0x3a, 0x31, 0x31, 0x00, 0x88, 0xf8, 0xff, 0xff,
      0x88, 0x07, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x76, 0xf8, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x70, 0xf8, 0xff, 0xff,
      0xac, 0xf8, 0xff, 0xff, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x18, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x3a, 0x54, 0x72, 0x61, 0x6e, 0x73, 0x70, 0x6f,
      0x73, 0x65, 0x3a, 0x32, 0x31, 0x00, 0x00, 0x00, 0xd4, 0xf8, 0xff, 0xff, 0x3c, 0x07, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0xc2, 0xf8, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0xbc, 0xf8, 0xff, 0xff, 0xf8, 0xf8, 0xff, 0xff,
      0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
      0x0c, 0x00, 0x00, 0x00, 0x3a, 0x49, 0x64, 0x65, 0x6e, 0x74, 0x69, 0x74, 0x79, 0x3a, 0x32, 0x31,
      0x00, 0x00, 0x00, 0x00, 0x20, 0xf9, 0xff, 0xff, 0x10, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0e, 0xf9, 0xff, 0xff,
      0x00, 0x00, 0x00, 0x01, 0x08, 0xf9, 0xff, 0xff, 0x44, 0xf9, 0xff, 0xff, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
      0x3a, 0x49, 0x64, 0x65, 0x6e, 0x74, 0x69, 0x74, 0x79, 0x3a, 0x31, 0x34, 0x00, 0x00, 0x00, 0x00,
      0x6c, 0xf9, 0xff, 0xff, 0xc4, 0x03, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x5a, 0xf9, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01,
      0x54, 0xf9, 0xff, 0xff, 0x90, 0xf9, 0xff, 0xff, 0x14, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
      0x63, 0x6f, 0x6d, 0x2e, 0x6d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x3a, 0x44, 0x65,
      0x71, 0x75, 0x61, 0x6e, 0x74, 0x69, 0x7a, 0x65, 0x4c, 0x69, 0x6e, 0x65, 0x61, 0x72, 0x3a, 0x31,
      0x00, 0x00, 0x00, 0x00, 0xd0, 0xf9, 0xff, 0xff, 0x60, 0x05, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xbe, 0xf9, 0xff, 0xff,
      0x00, 0x00, 0x00, 0x01, 0x78, 0xfa, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0xf8, 0xf9, 0xff, 0xff,
      0x68, 0x05, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x98, 0xfa, 0xff, 0xff, 0x02, 0x00, 0x00, 0x00, 0xe0, 0xf9, 0xff, 0xff,
      0x1c, 0xfa, 0xff, 0xff, 0x14, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      0x34, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x3a, 0x47, 0x61, 0x74,
      0x68, 0x65, 0x72, 0x3a, 0x31, 0x00, 0x00, 0x00, 0x44, 0xfa, 0xff, 0xff, 0x78, 0x02, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xe0, 0xfa, 0xff, 0xff,
      0x01, 0x00, 0x00, 0x00, 0x60, 0xfa, 0xff, 0xff, 0xb0, 0x05, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x4e, 0xfa, 0xff, 0xff,
      0x00, 0x00, 0x00, 0x01, 0x48, 0xfa, 0xff, 0xff, 0x84, 0xfa, 0xff, 0xff, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
      0x3a, 0x54, 0x72, 0x61, 0x6e, 0x73, 0x70, 0x6f, 0x73, 0x65, 0x3a, 0x31, 0x00, 0x00, 0x00, 0x00,
      0xac, 0xfa, 0xff, 0xff, 0x64, 0x05, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x9a, 0xfa, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01,
      0x94, 0xfa, 0xff, 0xff, 0xd0, 0xfa, 0xff, 0xff, 0x18, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x03, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00,
      0x12, 0x00, 0x00, 0x00, 0x3a, 0x51, 0x75, 0x61, 0x6e, 0x74, 0x69, 0x7a, 0x65, 0x4c, 0x69, 0x6e,
      0x65, 0x61, 0x72, 0x3a, 0x31, 0x33, 0x00, 0x00, 0x04, 0xfb, 0xff, 0xff, 0x2c, 0x04, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0xf2, 0xfa, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0xac, 0xfb, 0xff, 0xff, 0x02, 0x00, 0x00, 0x00,
      0x2c, 0xfb, 0xff, 0xff, 0x34, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x08, 0xfb, 0xff, 0xff, 0x44, 0xfb, 0xff, 0xff, 0x08, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x79, 0x5f, 0x73, 0x63, 0x61, 0x6c, 0x65, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xec, 0xfb, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00,
      0x6c, 0xfb, 0xff, 0xff, 0x14, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      0x1c, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x3a, 0x55, 0x6e, 0x73,
      0x71, 0x75, 0x65, 0x65, 0x7a, 0x65, 0x3a, 0x32, 0x31, 0x00, 0x00, 0x00, 0x98, 0xfb, 0xff, 0xff,
      0x78, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x86, 0xfb, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x80, 0xfb, 0xff, 0xff,
      0xbc, 0xfb, 0xff, 0xff, 0x18, 0x02, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x58, 0xfc, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0xd8, 0xfb, 0xff, 0xff,
      0x14, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
      0x38, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x3a, 0x47, 0x61, 0x74, 0x68, 0x65, 0x72, 0x3a,
      0x31, 0x33, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0x10, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xee, 0xfb, 0xff, 0xff,
      0x00, 0x00, 0x00, 0x01, 0xe8, 0xfb, 0xff, 0xff, 0x24, 0xfc, 0xff, 0xff, 0x98, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xc0, 0xfc, 0xff, 0xff,
      0x01, 0x00, 0x00, 0x00, 0x40, 0xfc, 0xff, 0xff, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x3a, 0x49, 0x64, 0x65,
      0x6e, 0x74, 0x69, 0x74, 0x79, 0x3a, 0x31, 0x39, 0x00, 0x00, 0x00, 0x00, 0x68, 0xfc, 0xff, 0xff,
      0xc8, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x56, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x50, 0xfc, 0xff, 0xff,
      0x8c, 0xfc, 0xff, 0xff, 0x14, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      0x40, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x3a, 0x47, 0x61, 0x74,
      0x68, 0x65, 0x72, 0x3a, 0x31, 0x31, 0x00, 0x00, 0xb4, 0xfc, 0xff, 0xff, 0x08, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x54, 0x69, 0x6e, 0x64, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x5c, 0xfd, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00,
      0xdc, 0xfc, 0xff, 0xff, 0x34, 0x03, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xca, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01,
      0xc4, 0xfc, 0xff, 0xff, 0x00, 0xfd, 0xff, 0xff, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x3a, 0x49, 0x64, 0x65,
      0x6e, 0x74, 0x69, 0x74, 0x79, 0x3a, 0x31, 0x36, 0x00, 0x00, 0x00, 0x00, 0x28, 0xfd, 0xff, 0xff,
      0x08, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x1e, 0xfd, 0xff, 0xff,
      0x00, 0x00, 0x00, 0x01, 0x18, 0xfd, 0xff, 0xff, 0x54, 0xfd, 0xff, 0xff, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
      0x3a, 0x54, 0x72, 0x61, 0x6e, 0x73, 0x70, 0x6f, 0x73, 0x65, 0x3a, 0x31, 0x33, 0x00, 0x00, 0x00,
      0x7c, 0xfd, 0xff, 0xff, 0x94, 0x02, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6a, 0xfd, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01,
      0x64, 0xfd, 0xff, 0xff, 0xa0, 0xfd, 0xff, 0xff, 0x14, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
      0x3a, 0x55, 0x6e, 0x73, 0x71, 0x75, 0x65, 0x65, 0x7a, 0x65, 0x3a, 0x31, 0x33, 0x00, 0x00, 0x00,
      0xcc, 0xfd, 0xff, 0xff, 0x08, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x61, 0x78, 0x65, 0x73, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x74, 0xfe, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0xf4, 0xfd, 0xff, 0xff, 0x1c, 0x02, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0xe2, 0xfd, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0xdc, 0xfd, 0xff, 0xff, 0x18, 0xfe, 0xff, 0xff,
      0x14, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00,
      0x1c, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x3a, 0x51, 0x75, 0x61, 0x6e, 0x74, 0x69, 0x7a,
      0x65, 0x4c, 0x69, 0x6e, 0x65, 0x61, 0x72, 0x3a, 0x31, 0x39, 0x00, 0x00, 0x48, 0xfe, 0xff, 0xff,
      0xe8, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x36, 0xfe, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0xf0, 0xfe, 0xff, 0xff,
      0x02, 0x00, 0x00, 0x00, 0x70, 0xfe, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0xff, 0xff, 0xff,
      0x01, 0x00, 0x00, 0x00, 0x58, 0xfe, 0xff, 0xff, 0x94, 0xfe, 0xff, 0xff, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00,
      0x63, 0x6f, 0x6d, 0x2e, 0x6d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x3a, 0x4e, 0x68,
      0x77, 0x63, 0x4d, 0x61, 0x78, 0x50, 0x6f, 0x6f, 0x6c, 0x3a, 0x31, 0x00, 0xc8, 0xfe, 0xff, 0xff,
      0x48, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0xb6, 0xfe, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0xb0, 0xfe, 0xff, 0xff,
      0xec, 0xfe, 0xff, 0xff, 0x14, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      0x5c, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x63, 0x6f, 0x6d, 0x2e,
      0x6d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x3a, 0x51, 0x75, 0x61, 0x6e, 0x74, 0x69,
      0x7a, 0x65, 0x4c, 0x69, 0x6e, 0x65, 0x61, 0x72, 0x3a, 0x31, 0x00, 0x00, 0x28, 0xff, 0xff, 0xff,
      0x08, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x54, 0x32, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x1e, 0xff, 0xff, 0xff,
      0x00, 0x00, 0x00, 0x01, 0xd8, 0xff, 0xff, 0xff, 0x02, 0x00, 0x00, 0x00, 0x58, 0xff, 0xff, 0xff,
      0x08, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x54, 0x31, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00,
      0x00, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x50, 0xff, 0xff, 0xff,
      0x8c, 0xff, 0xff, 0xff, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x18, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x3a, 0x49, 0x64, 0x65, 0x6e, 0x74, 0x69, 0x74,
      0x79, 0x3a, 0x31, 0x33, 0x00, 0x00, 0x00, 0x00, 0xb4, 0xff, 0xff, 0xff, 0x5c, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0xa2, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x9c, 0xff, 0xff, 0xff, 0xd8, 0xff, 0xff, 0xff,
      0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
      0x0c, 0x00, 0x00, 0x00, 0x3a, 0x55, 0x6e, 0x73, 0x71, 0x75, 0x65, 0x65, 0x7a, 0x65, 0x3a, 0x31,
      0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x0c, 0x00, 0x04, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00,
      0x08, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00,
      0x08, 0x00, 0x07, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00, 0x04, 0x00,
      0x04, 0x00, 0x00, 0x00,
  };
  // clang-format on

  ORT_RETURN_IF_ERROR(LoadKernelTypeStrResolverFromBuffer(resolver_with_required_ops,
                                                          kLayoutTransformationRequiredOpsKernelTypeStrResolverBytes));
  kernel_type_str_resolver.Merge(std::move(resolver_with_required_ops));
  return Status::OK();
}

}  // namespace onnxruntime::kernel_type_str_resolver_utils

#endif  // !defined(ORT_MINIMAL_BUILD) || defined(ORT_EXTENDED_MINIMAL_BUILD)
