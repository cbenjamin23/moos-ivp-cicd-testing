#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "UtterQueue.h"

namespace {

Utterance MakeUtterance(const std::string& text, double priority)
{
  Utterance utter;
  utter.setText(text);
  utter.setPriority(priority);
  return utter;
}

}  // namespace

// Source audit note: this suite covers UtterQueue ordering, insertion-order
// priority adjustment, pushTop behavior, empty pop, clear semantics, and copy
// snapshots without touching Sayer's audio system commands.

TEST(UtterQueueTest, EmptyPopReturnsEmptyUtterance)
{
  UtterQueue queue;

  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0u);
  EXPECT_TRUE(queue.pop().isEmpty());
}

TEST(UtterQueueTest, PopsHigherPriorityBeforeLowerPriority)
{
  UtterQueue queue;
  queue.push(MakeUtterance("low", 1));
  queue.push(MakeUtterance("high", 5));
  queue.push(MakeUtterance("mid", 3));

  EXPECT_EQ(queue.pop().getText(), "high");
  EXPECT_EQ(queue.pop().getText(), "mid");
  EXPECT_EQ(queue.pop().getText(), "low");
  EXPECT_TRUE(queue.empty());
}

TEST(UtterQueueTest, EqualPriorityEntriesPreserveInsertionOrderByAdjustment)
{
  UtterQueue queue;
  queue.push(MakeUtterance("first", 10));
  queue.push(MakeUtterance("second", 10));
  queue.push(MakeUtterance("third", 10));

  EXPECT_EQ(queue.pop().getText(), "first");
  EXPECT_EQ(queue.pop().getText(), "second");
  EXPECT_EQ(queue.pop().getText(), "third");
}

TEST(UtterQueueTest, PushStoresPriorityWithSmallInsertionPenalty)
{
  UtterQueue queue;
  queue.push(MakeUtterance("first", 7));
  queue.push(MakeUtterance("second", 7));

  const Utterance first = queue.pop();
  const Utterance second = queue.pop();

  EXPECT_DOUBLE_EQ(first.getPriority(), 7);
  EXPECT_LT(second.getPriority(), 7);
  EXPECT_GT(second.getPriority(), 6.999);
}

TEST(UtterQueueTest, PushTopPlacesNewEntryAboveCurrentTop)
{
  UtterQueue queue;
  queue.push(MakeUtterance("base", 2));
  queue.push(MakeUtterance("higher", 4));
  queue.pushTop(MakeUtterance("top", 0));

  EXPECT_EQ(queue.pop().getText(), "top");
  EXPECT_EQ(queue.pop().getText(), "higher");
  EXPECT_EQ(queue.pop().getText(), "base");
}

TEST(UtterQueueTest, PushTopOnEmptyQueueBehavesLikePush)
{
  UtterQueue queue;
  queue.pushTop(MakeUtterance("only", 2));

  ASSERT_EQ(queue.size(), 1u);
  EXPECT_EQ(queue.pop().getText(), "only");
}

TEST(UtterQueueTest, CopyAllEntriesReturnsPriorityOrderWithoutMutation)
{
  UtterQueue queue;
  queue.push(MakeUtterance("low", 1));
  queue.push(MakeUtterance("high", 3));
  queue.push(MakeUtterance("mid", 2));

  const std::vector<Utterance> copy = queue.getCopyAllEntries();

  ASSERT_EQ(copy.size(), 3u);
  EXPECT_EQ(copy[0].getText(), "high");
  EXPECT_EQ(copy[1].getText(), "mid");
  EXPECT_EQ(copy[2].getText(), "low");
  EXPECT_EQ(queue.size(), 3u);
}

TEST(UtterQueueTest, ClearRemovesEntriesButKeepsInsertionCounterEffect)
{
  UtterQueue queue;
  queue.push(MakeUtterance("old", 5));
  queue.clear();

  EXPECT_TRUE(queue.empty());

  queue.push(MakeUtterance("new", 5));
  const Utterance popped = queue.pop();
  EXPECT_EQ(popped.getText(), "new");
  EXPECT_LT(popped.getPriority(), 5);
}
