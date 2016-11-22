#ifndef FAKETIMER_H
#define FAKETIMER_H

class fakeTimer_t {

public:
  bool elapsed;

  void init( uint32_t period, void(* callback)(void) ) {
    p = period;
    if ( callback != NULL )
      fcn = callback;
  }

  void start() {
    nextAlm = micros() + p;
    on = true;
  }

  void stop() {
    on = false;
  }

  void reset() {
    nextAlm = micros() + p;
  }

  void reinit( uint32_t period, void(* callback)(void) ) {
    init(period, callback);
    reset();
  }

  bool check() {
    if ( on ) {
      if ( micros() > nextAlm ) {
        elapsed = true;
        nextAlm += p;
        fcn();
      } else {
        elapsed = false;
      }
      return elapsed;
    } 
    return false;
  }

private:
  uint32_t    nextAlm;
  bool        on;
  uint32_t    p;
  void(* fcn)(void);

} ;

#endif // FAKETIMER_H
