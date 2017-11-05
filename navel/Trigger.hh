#ifndef navel_Products_Trigger_hh
#define navel_Products_Trigger_hh

#include <vector>

namespace next
{

  ///
  /// Class to contain information about the trigger conditions for
  /// the run in question. Holds information like the type of trigger
  /// Buffer size etc. (should it contain DAQ version?)
  ///

class Trigger
{
public:

  // Default constructor.
  Trigger();

  // Constructor with trigger type.
  Trigger(int trigger);

  // Getters and setters
  int triggerType() const;
  void setTriggerType(int trigger);

  double DAQbufferSize() const;
  void setDAQbufferSize(double buffer);

  double preTrigger() const;
  void setPreTrigger(double pretrigger);

  int fecID() const;
  void setFecID(int fecid);

  int bufferPosition() const;
  void setBufferPosition(int bufferposition);

  int triggerCode() const;
  void setTriggerCode(int trigger);

  bool mask() const;
  void setMask(bool mask);

  int triggerMask() const;
  void setTriggerMask(int mask);

  bool externalTrigger() const;
  void setExternalTrigger(bool external);

  bool dualTrigger() const;
  void setDualTrigger(bool dual);

  bool autoTrigger() const;
  void setAutoTrigger(bool dual);

  bool twoTrigger() const;
  void setTwoTrigger(bool two);

  int diff() const;
  void setDiff(int diff);

  int chan1() const;
  void setChan1(int chan1);

  int chan2() const;
  void setChan2(int chan2);

  int window1() const;
  void setWindow1(int window1);

  int window2() const;
  void setWindow2(int window2);

  int triggerIntN() const;
  void setTriggerIntN(int intN);

  int triggerExtN() const;
  void setTriggerExtN(int extN);

  unsigned int nTrigChan() const;
  void setTrigChan(const std::vector<int>& trCh);
  void addTrigChan(int trCh);
  void getTrigChans(std::vector<int>& trCh) const;
  int getTrigChan(int i) const;

private:

  int trigger_; ///< Trigger type (pos vals?).
  double DAQbufferSize_; ///< Size in us of the circular buffer.
  double preTrigger_; ///< number of us of the buffer before the trigger.
  int fecID_; ///< ID of the front end card associated with this trigger type;
  int bufferPos_; ///< memory position of the trigger in the FEC

  std::vector<int> trigChan_; ///< Channels where trigger occurred.

  // Fields from Trigger configuration register
  int triggerCode_; // Trigger code assigned to RUN
  int triggerDiff_; // Trigger 1/2 maximum time difference to trigger
  int triggerMask_; // Trigger mask value (steps of 25 ns)
  bool mask_; // Trigger mask on/off
  bool externalTrigger_; // External trigger activation
  bool dualTrigger_; // Dual internal/external trigger
  bool twoTrigger_; // Two consecutive triggers to trigger
  bool autoTrigger_; // Auto external trigger activation
  int triggerChan1_; // Number of trigger events needed to generate a trigger1 (up to 40)
  int triggerChan2_; // Number of trigger events needed to generate a trigger2 (up to 40)
  int triggerWindow1_; //Coincidence window 1 (up to 64 time bins of 25ns)
  int triggerWindow2_; //Coincidence window 2 (up to 64 time bins of 25ns)
  int triggerIntN_; // Number of internal triggers in dual mode to switch to external
  int triggerExtN_; // Number of external triggers in dual mode to switch to internal


}; // class Trigger.

  typedef std::vector<Trigger> TriggerCollection;

  /// INLINE METHODS
  inline int Trigger::triggerType() const { return trigger_; }
  inline void Trigger::setTriggerType(int trigger){ trigger_ = trigger; }

  inline double Trigger::DAQbufferSize() const { return DAQbufferSize_; }
  inline void Trigger::setDAQbufferSize(double buffer){ DAQbufferSize_ = buffer; }

  inline double Trigger::preTrigger() const { return preTrigger_; }
  inline void Trigger::setPreTrigger(double pretrigger) { preTrigger_ = pretrigger; }

  inline int Trigger::fecID() const { return fecID_; }
  inline void Trigger::setFecID(int fecid) { fecID_ = fecid; }

  inline int Trigger::bufferPosition() const { return bufferPos_; }
  inline void Trigger::setBufferPosition(int bufferposition) { bufferPos_ = bufferposition; }

  inline unsigned int Trigger::nTrigChan() const { return trigChan_.size(); }
  inline int Trigger::getTrigChan(int i) const { return trigChan_.at(i); }

  inline int Trigger::triggerCode() const { return triggerCode_; }
  inline void Trigger::setTriggerCode(int trigger){ triggerCode_ = trigger; }

  inline bool Trigger::mask() const { return mask_; }
  inline void Trigger::setMask(bool mask) { mask_ = mask; }

  inline int Trigger::triggerMask() const { return triggerMask_; }
  inline void Trigger::setTriggerMask(int mask) { triggerMask_ = mask; }

  inline bool Trigger::externalTrigger() const { return externalTrigger_; }
  inline void Trigger::setExternalTrigger(bool external) { externalTrigger_ = external; }

  inline bool Trigger::dualTrigger() const { return dualTrigger_; }
  inline void Trigger::setDualTrigger(bool dual) { dualTrigger_ = dual; }

  inline bool Trigger::autoTrigger() const { return autoTrigger_; }
  inline void Trigger::setAutoTrigger(bool autoTrigger) { autoTrigger_ = autoTrigger; }

  inline bool Trigger::twoTrigger() const { return twoTrigger_; }
  inline void Trigger::setTwoTrigger(bool two) { twoTrigger_ = two; }

  inline int Trigger::diff() const { return triggerDiff_; }
  inline void Trigger::setDiff(int diff) { triggerDiff_ = diff; }

  inline int Trigger::chan1() const { return triggerChan1_; }
  inline void Trigger::setChan1(int chan1) { triggerChan1_ = chan1; }

  inline int Trigger::chan2() const { return triggerChan2_; }
  inline void Trigger::setChan2(int chan2) { triggerChan2_ = chan2; }

  inline int Trigger::window1() const { return triggerWindow1_; }
  inline void Trigger::setWindow1(int window1) { triggerWindow1_ = window1; }

  inline int Trigger::window2() const { return triggerWindow2_; }
  inline void Trigger::setWindow2(int window2) { triggerWindow2_ = window2; }

  inline int Trigger::triggerIntN() const { return triggerIntN_; }
  inline void Trigger::setTriggerIntN(int intN) { triggerIntN_ = intN; }

  inline int Trigger::triggerExtN() const { return triggerExtN_; }
  inline void Trigger::setTriggerExtN(int extN) { triggerExtN_ = extN; }

  } //namespace next

#endif
