#include "torch/csrc/jit/passes/peephole.h"

namespace torch { namespace jit {

// The intent for this optimization pass is to catch all of the small, easy to
// catch peephole optimizations you might be interested in doing.
//
// Right now, it does:
//    - Eliminate no-op 'expand' nodes
//    - Simply x.t().t() to x
//
// TODO: Decide what kind of fixed point strategy we will have
void PeepholeOptimize(std::shared_ptr<Graph>& graph) {
  for (auto it = graph->nodes().begin(); it != graph->nodes().end(); ++it) {
    auto* n = *it;

    switch (n->kind()) {
      case kexpand:
        // Eliminate redundant expand
        if (!n->input()->hasType()) break;
        if (n->is(ksize) == n->input()->type()->expect<TensorType>()->sizes()) {
          n->output()->replaceAllUsesWith(n->input());
          it.destroyCurrent();
        }
        break;
      case kt:
        // x.t().t() == x
        auto input_node = n->input()->node();
        if (input_node->kind() == kt)  {
          n->output()->replaceAllUsesWith(input_node->input());
          it.destroyCurrent();
          // The previous transpose might be unnecessary now.
          if (input_node->output()->uses().size() == 0) {
            if (*it == input_node) {
              it.destroyCurrent();
            } else {
              input_node->destroy();
            }
          }
        }
        break;
    }
  }
}

}}
