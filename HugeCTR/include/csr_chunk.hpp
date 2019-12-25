/*
 * Copyright (c) 2019, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <vector>
#include "HugeCTR/include/csr.hpp"

namespace HugeCTR {

/**
 * @brief A wrapper of CSR objects.
 *
 * For each iteration DataReader will get one chunk of CSR objects from Heap
 * Such a chunk contains the training input data (sample + label) required by
 * this iteration
 */
template <typename CSR_Type>
class CSRChunk {
 private:
  std::vector<CSR<CSR_Type>>
      csr_buffers_; /**< A vector of CSR objects, should be same number as devices. */
  std::vector<PinnedBuffer<float>> label_buffers_; /**< A vector of label buffers */
  int label_dim_;                                  /**< dimension of label (for one sample) */
  int slot_num_;                                   /**< slot num */
  int batchsize_;                                  /**< batch size of training */
 public:
  /**
   * Ctor of CSRChunk.
   * Create and initialize the CSRChunk
   * @param num_csr_buffers the number of CSR object it will have.
   *        the number usually equal to num devices will be used.
   * @param batchsize batch size.
   * @param label_dim dimension of label (for one sample).
   * @param slot_num slot num.
   * @param max_value_size the number of element of values the CSR matrix will have
   *        for num_rows rows (See csr.hpp).
   */
  CSRChunk(int num_csr_buffers, int batchsize, int label_dim, int slot_num, int max_value_size) {
    if (num_csr_buffers <= 0 || batchsize % num_csr_buffers != 0 || label_dim <= 0 ||
        slot_num <= 0 || max_value_size <= batchsize) {
      CK_THROW_(Error_t::WrongInput,
                "num_src_buffers <= 0 || batchsize%num_csr_buffers != 0 || label_dim <= 0 ||  "
                "slot_num <=0 || max_value_size <= batchsize");
    }
    if (batchsize % num_csr_buffers != 0)
      CK_THROW_(Error_t::WrongInput, "batchsize%num_csr_buffers");
    label_dim_ = label_dim;
    batchsize_ = batchsize;
    slot_num_ = slot_num;
    assert(csr_buffers_.empty() && label_buffers_.empty());
    for (int i = 0; i < num_csr_buffers; i++) {
      csr_buffers_.emplace_back(batchsize * slot_num, max_value_size);
      label_buffers_.emplace_back(batchsize / num_csr_buffers * label_dim);
    }
  }

  /**
   * Get the vector of csr objects.
   * This methord is used in collector (consumer) and data_reader (provider).
   */
  const std::vector<CSR<CSR_Type>>& get_csr_buffers() const { return csr_buffers_; }

  /**
   * Get the specific csr object.
   * This methord is used in collector (consumer) and data_reader (provider).
   */
  CSR<CSR_Type>& get_csr_buffer(int i) { return csr_buffers_[i]; }

  /**
   * Call member function of all csr objects.
   * This methord is used in collector (consumer) and data_reader (provider).
   */
  template <typename MemberFunctionPointer>
  void apply_to_csr_buffers(const MemberFunctionPointer& fp) {
    for (auto& csr_buffer : csr_buffers_) {
      (csr_buffer.*fp)();
    }
  }

  /**
   * Get labels
   * This methord is used in collector (consumer) and data_reader (provider).
   */
  const std::vector<PinnedBuffer<float>>& get_label_buffers() const { return label_buffers_; }
  int get_label_dim() const { return label_dim_; }
  int get_batchsize() const { return batchsize_; }
  int get_slot_num() const { return slot_num_; }

  /**
   * A copy Ctor but allocating new resources.
   * This Ctor is used in Heap (Ctor) to make several
   * copies of the object in heap.
   * @param C prototype of the Ctor.
   */
  CSRChunk(const CSRChunk&) = delete;
  CSRChunk& operator=(const CSRChunk&) = delete;
  CSRChunk(CSRChunk&&) = default;
};

}  // namespace HugeCTR