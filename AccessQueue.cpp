// AccessQueue.cpp

#include "pch.h"


AccessQueue::AccessQueue(const std::string &CallSign)
{
  // CallSign is just for debug output purposes;
  this->CallSign = CallSign;
}

AccessQueue::~AccessQueue()
{
  for(auto ListItr = Queue.begin(), ListEnd = Queue.end();
      ListItr != ListEnd; ListItr++)
    delete *ListItr;
}

AccessQueue::QueueToken AccessQueue::Lock(bool WriteAccess, const unsigned int &Prio)
{
  std::unique_lock<std::mutex> Lock(Mutex);

  // Create an identifier that is unique for this thread.  Why not a memory address?  It's unique for this thread.
  // Also, we can store the prio in the addressed memory, so yay.
  // Put it on the queue.
  std::pair<unsigned int, bool> * Identifier = new std::pair<unsigned int, bool>(Prio, WriteAccess);
  InsertIntoQueue(Identifier);

  return GetTurn(Identifier, Lock);
}

AccessQueue::QueueToken AccessQueue::GetTurn(const std::pair<unsigned int, bool> * Identifier,
                                             std::unique_lock<std::mutex> &Lock)
{
  // Find out if it is our turn (Will be if the queue was empty before we got here and noone used the ressource.
  // If it is our turn, we crack right on ahead, if not, we wait.
  bool MyTurn = IsUpNext(Identifier);
  
  if(!MyTurn)
    CV.wait(Lock, [this, &Identifier]{return this->IsUpNext(Identifier);});

  // Now that it is our turn, we remove ourselves from the queue and put ourselves in the used by list.
  Queue.erase(std::find(Queue.begin(), Queue.end(), Identifier));
  UsedBy.push_back(Identifier);

  // Now check if this access is a read-only.  If so, look to see if the next one in queue is also read-only.
  if(!Identifier->second && !Queue.front()->second)
    CV.notify_all();
    
  QueueToken Token = QueueToken(Identifier, this);

  // Return the identifier so the calling function can keep it and let us know it later for clean up.
  // This allows for sanity checks.
  return Token;
}  

void AccessQueue::Yield(AccessQueue::QueueToken &Token)
{
  std::unique_lock<std::mutex> Lock(Mutex);

  // Insert the Identifier into the queue and remove it from using.
  InsertIntoQueue(Token.GetIdentifier());
  UsedBy.erase(std::find(UsedBy.begin(), UsedBy.end(), Token.GetIdentifier()));

  CV.notify_all();

  Token = GetTurn(Token.GetIdentifier(), Lock);
}


void AccessQueue::InsertIntoQueue(const std::pair<unsigned int, bool> * Identifier)
{
  auto ListItr = Queue.begin(), EndItr = Queue.end();
  for(; (ListItr != EndItr) && ((**ListItr).first<=(*Identifier).first); ListItr++);
  Queue.insert(ListItr, Identifier);
}


bool AccessQueue::IsUpNext(const std::pair<unsigned int, bool> * Identifier)
{
  // Don't go if the data is in use with Write Access.
  if(UsedBy.size() && UsedBy.front()->second)
    return false;

  // Don't go if the data is in use and we want write access.
  if(UsedBy.size() && Identifier->second)
    return false;

  bool NotNext = false;
  auto ThisSpot = std::find(Queue.begin(), Queue.end(), Identifier);

  // First find out if the queue contains any ticket with lower prio;
  for(auto ListItr = Queue.begin(), EndItr = Queue.end();
      (ListItr != EndItr) && !NotNext; ListItr++)
  {
    NotNext |= (**ListItr).first < (*Identifier).first;
  }

  if(NotNext)
    return false;

  // We did not find anyone with lower prio.  Now theck if anyone with equal prio is in front of us.
  for(auto ListItr = Queue.begin();
      (ListItr != ThisSpot) && !NotNext; ListItr++)
    NotNext |= (**ListItr).first == (*Identifier).first;
  
  return !NotNext;
}


void AccessQueue::Unlock(const std::pair<unsigned int, bool> *Identifier)
{
  // Clean up after the run by freeing up the memory, setting UsedBy to false and signal any other threads.
  std::lock_guard<std::mutex> Lock(Mutex);

  // First, sanity check.
  if(!UsedBy.size() || std::find(UsedBy.begin(), UsedBy.end(), Identifier) == UsedBy.end())
  {
    std::printf("Shit has happened.  An invalied attempt to unlock an AccessQueue has been attempted.\n");
    if(CallSign.size())
    {
      std::printf("The AccessQueue has the CallSign %s\n", CallSign.c_str());
    }
    exit(-1);
  }

  // Now remove the identifier from places.
  UsedBy.erase(std::find(UsedBy.begin(), UsedBy.end(), Identifier));
  delete Identifier;

  CV.notify_all();
}

std::string AccessQueue::GetCallSign()
{
  return CallSign;
}

int AccessQueue::GetQueueLength()
{
  std::unique_lock<std::mutex> Lock(Mutex);
  return Queue.size();
}

int AccessQueue::GetQueueLength(const QueueToken &Token)
{
  std::unique_lock<std::mutex> Lock(Mutex);
  int Length = 0;
  for(auto QueueItr = Queue.begin(), QueueEnd = Queue.end(); 
      QueueItr != QueueEnd && *QueueItr != Token.GetIdentifier(); QueueItr++)
  {
    Length++;
  }
  return Length;
}
