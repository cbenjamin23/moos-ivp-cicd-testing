#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Threadsafe_pipe.h"

namespace {

struct MissionEvent {
  std::string app;
  std::string event;
  int iteration;
};

}  // namespace

// Covers threadsafe pipe behavior: dequeues in queue order for mission events.
TEST(ThreadsafePipeTest, DequeuesInQueueOrderForMissionEvents)
{
  Threadsafe_pipe<std::string> pipe;
  EXPECT_TRUE(pipe.empty());
  EXPECT_TRUE(pipe.enqueue("OnStartUp"));
  EXPECT_TRUE(pipe.enqueue("OnNewMail:NODE_REPORT"));
  EXPECT_TRUE(pipe.enqueue("Iterate"));
  EXPECT_FALSE(pipe.empty());

  std::string value;
  EXPECT_TRUE(pipe.dequeue(value));
  EXPECT_EQ(value, "OnStartUp");
  EXPECT_TRUE(pipe.dequeue(value));
  EXPECT_EQ(value, "OnNewMail:NODE_REPORT");
  EXPECT_FALSE(pipe.empty());
  EXPECT_TRUE(pipe.dequeue(value));
  EXPECT_EQ(value, "Iterate");
  EXPECT_TRUE(pipe.empty());
}

// Covers threadsafe pipe behavior: close rejects future enqueues but drains existing values.
TEST(ThreadsafePipeTest, CloseRejectsFutureEnqueuesButDrainsExistingValues)
{
  Threadsafe_pipe<int> pipe;
  EXPECT_TRUE(pipe.enqueue(1));
  EXPECT_TRUE(pipe.enqueue(2));
  pipe.close();
  EXPECT_FALSE(pipe.enqueue(3));

  int value = 0;
  EXPECT_TRUE(pipe.dequeue(value));
  EXPECT_EQ(value, 1);
  EXPECT_TRUE(pipe.dequeue(value));
  EXPECT_EQ(value, 2);
  EXPECT_FALSE(pipe.dequeue(value));
  EXPECT_TRUE(pipe.empty());
}

// Covers threadsafe pipe behavior: close is idempotent and keeps rejecting enqueues.
TEST(ThreadsafePipeTest, CloseIsIdempotentAndKeepsRejectingEnqueues)
{
  Threadsafe_pipe<std::string> pipe;
  EXPECT_TRUE(pipe.enqueue("initial"));
  pipe.close();
  pipe.close();

  EXPECT_FALSE(pipe.enqueue("after-close"));
  std::string value;
  EXPECT_TRUE(pipe.dequeue(value));
  EXPECT_EQ(value, "initial");
  EXPECT_FALSE(pipe.dequeue(value));
  EXPECT_FALSE(pipe.enqueue("after-drain"));
}

// Covers threadsafe pipe behavior: supports structured mission payload copies.
TEST(ThreadsafePipeTest, SupportsStructuredMissionPayloadCopies)
{
  Threadsafe_pipe<MissionEvent> pipe;
  MissionEvent event{"pHelmIvP", "OnNewMail:DEPLOY=true", 42};
  EXPECT_TRUE(pipe.enqueue(event));

  event.app = "mutated-locally";
  pipe.close();

  MissionEvent out{"", "", 0};
  EXPECT_TRUE(pipe.dequeue(out));
  EXPECT_EQ(out.app, "pHelmIvP");
  EXPECT_EQ(out.event, "OnNewMail:DEPLOY=true");
  EXPECT_EQ(out.iteration, 42);
  EXPECT_FALSE(pipe.dequeue(out));
}

// Covers threadsafe pipe behavior: preserves multiple structured events through close.
TEST(ThreadsafePipeTest, PreservesMultipleStructuredEventsThroughClose)
{
  Threadsafe_pipe<MissionEvent> pipe;
  EXPECT_TRUE(pipe.enqueue({"pMarineViewer", "OnStartUp", 0}));
  EXPECT_TRUE(pipe.enqueue({"pMarineViewer", "OnNewMail:NODE_REPORT", 1}));
  EXPECT_TRUE(pipe.enqueue({"pMarineViewer", "Iterate", 2}));
  pipe.close();

  MissionEvent out{"", "", -1};
  ASSERT_TRUE(pipe.dequeue(out));
  EXPECT_EQ(out.event, "OnStartUp");
  EXPECT_EQ(out.iteration, 0);
  ASSERT_TRUE(pipe.dequeue(out));
  EXPECT_EQ(out.event, "OnNewMail:NODE_REPORT");
  EXPECT_EQ(out.iteration, 1);
  ASSERT_TRUE(pipe.dequeue(out));
  EXPECT_EQ(out.event, "Iterate");
  EXPECT_EQ(out.iteration, 2);
  EXPECT_FALSE(pipe.dequeue(out));
  EXPECT_EQ(out.event, "Iterate");
}

// Covers threadsafe pipe behavior: empty reflects queue state before and after close.
TEST(ThreadsafePipeTest, EmptyReflectsQueueStateBeforeAndAfterClose)
{
  Threadsafe_pipe<int> pipe;
  EXPECT_TRUE(pipe.empty());
  EXPECT_TRUE(pipe.enqueue(7));
  EXPECT_FALSE(pipe.empty());
  pipe.close();
  EXPECT_FALSE(pipe.empty());

  int value = 0;
  EXPECT_TRUE(pipe.dequeue(value));
  EXPECT_EQ(value, 7);
  EXPECT_TRUE(pipe.empty());
}

// Covers threadsafe pipe behavior: empty closed pipe dequeues false.
TEST(ThreadsafePipeTest, EmptyClosedPipeDequeuesFalse)
{
  Threadsafe_pipe<std::string> pipe;
  EXPECT_TRUE(pipe.empty());
  pipe.close();
  std::string value = "unchanged";
  EXPECT_FALSE(pipe.dequeue(value));
  EXPECT_EQ(value, "unchanged");
}

// Covers threadsafe pipe behavior: close after drain prevents further GUI event posts.
TEST(ThreadsafePipeTest, CloseAfterDrainPreventsFurtherGuiEventPosts)
{
  Threadsafe_pipe<std::string> pipe;
  ASSERT_TRUE(pipe.enqueue("OnStartUp"));

  std::string value;
  ASSERT_TRUE(pipe.dequeue(value));
  EXPECT_EQ(value, "OnStartUp");
  EXPECT_TRUE(pipe.empty());

  pipe.close();
  EXPECT_FALSE(pipe.enqueue("Iterate"));
  value = "unchanged";
  EXPECT_FALSE(pipe.dequeue(value));
  EXPECT_EQ(value, "unchanged");
}
