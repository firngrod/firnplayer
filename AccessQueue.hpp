// AccessQueue.hpp
#ifndef _ACCESSQUEUE_HPP_
#define _ACCESSQUEUE_HPP_

class AccessQueue
{
  // Helper class for holding queue tokens.
public:
  class QueueToken
  {
  friend AccessQueue;
  public:
    ~QueueToken();
    std::string GetCallSign();
    int GetQueueLength();
    int GetPrioQueueLength();
    const std::pair<unsigned int, bool> * GetIdentifier() const;
    void Unlock();
    void Relock();
    void Yield();

  protected:
    QueueToken(const std::pair<unsigned int, bool> * Identifier, AccessQueue *ParentQueue);
    const std::pair<unsigned int, bool> * Identifier;
    std::string CallSign;
    AccessQueue *ParentQueue;
    bool WriteAccess;
    unsigned int Prio;
  };

  friend QueueToken;
public:
  // Queue can be given a name so that debugging can be traced tothe correct queue.
  AccessQueue(const std::string &Callsign = "");
  ~AccessQueue();

  // Getting in queue is done with a priority.  Any time queue positions are resolved, the entrant
  // with the lowest prio number is taken.  In case of conflict, first come, first served.
  // An unsigned int pointer is returned.  This is used internally as the entrant's ID in the queue.
  // The pointer points to an unsigned int containing the priority of the ticket.  The pointer itself
  // is the ID since it is guaranteed to be unique.
  // The ID needs to be passed to the queue when releasing the queue to prevent unautherized ressource
  // release.
  // The complexity of handling priorities makes the hand-over between threads take longer the more
  // threads are in the queue.  This has been attempted handled in various ways, but still becomes significant
  // at some point.
  QueueToken Lock(bool WriteAccess = true, const unsigned int &Prio = 100);
  void Unlock(const std::pair<unsigned int, bool> *Identifier);
  int GetQueueLength();
  int GetQueueLength(const QueueToken &Token);
  std::string GetCallSign();
  AccessQueue::QueueToken GetTurn(const std::pair<unsigned int, bool> * Identifier, 
                                  std::unique_lock<std::mutex> &Lock);

private:
  std::list<const std::pair<unsigned int, bool> *>  Queue;
  std::list<const std::pair<unsigned int, bool> *>  UsedBy;
  std::mutex                    Mutex;
  std::condition_variable       CV;
  void                          InsertIntoQueue(const std::pair<unsigned int, bool> * Identifier);
  bool                          IsUpNext(const std::pair<unsigned int, bool> * Identifier);
  std::string                   CallSign;
  void                          Yield(AccessQueue::QueueToken &Token);

};

#endif // _ACCESSQUEUE_HPP_
