#include "pzem_edl.hpp"

class FakeMeterPZ003 {
public:
    // variances
    pz003::metrics mt;
    // randomise deviation thresholds (percents)
    var_t deviate = var_t(8, 30, 3, 20);
    // probability of randomizing a value on each poll, in 1/x
    var_t prob = var_t(10, 5, 15, 10);

    /**
     * @brief reset all values to defaults
     * 
     */
    void reset();

    /**
     * @brief toss metering variables
     * using deviation and probability settings 
     * 
     */
    void randomize(pz003::metrics& m);

    /**
     * @brief recalculate power/energy values based on time elapsed and current metrics
     * 
     */
    void updnrg(pz003::metrics& m);

protected:
    uint64_t timecount = 0;     // in ms
    uint32_t _nrg = 0;          // energy

};


/**
 * @brief A fake PZEM004 device class
 * it pretends to be PZEM004 class but does not talk via serial to the real device,
 * it just provides some random meterings. Could be used for PZEM software prototyping
 * without the need for real device.
 * Not all functions supported (yet), i.e. alarms.
 * 
 */
class DummyPZ00e : public PZ003 {

public:
    FakeMeterPZ003 fm;

    // Derrived constructor
    DummyPZ003(const uint8_t _id,  uint8_t modbus_addr = ADDR_ANY, const char *_descr = nullptr) : PZ003(_id, modbus_addr, _descr) { fm.reset(); pz.data = fm.mt; }

    //virtual d-tor
    virtual ~DummyPZ003(){};

    // Override methods
    void resetEnergyCounter() override { pz.data.energy = 0; fm.reset(); };

    void updateMetrics() override;

    /**
     * @brief a "do nothing" sink
     * I do not expect any real data to be fed to dummy device
     */
    void rx_sink(const RX_msg *msg) override {};

    /* ***  *** */
    // Own methods

    // reset energy counter to some specific value
    void resetEnergyCounter(uint32_t e){ pz.data.energy = e; fm.mt.energy = e; };
};
#endif // ARDUINO
