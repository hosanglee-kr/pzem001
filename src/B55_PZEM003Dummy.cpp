

#ifdef ARDUINO
void FakeMeterPZ003::reset(){
    mt.voltage = DEF_U;
    mt.current = DEF_I;
    mt.freq = DEF_FREQ;
    mt.pf = DEF_PF;

    mt.power = mt.voltage * mt.current * mt.pf / 100000;
    timecount = esp_timer_get_time() >> 10;
    _nrg = 0;
}

void FakeMeterPZ003::randomize(pz003::metrics& m){
    // voltage
    if (random(prob.voltage) == prob.voltage-1){
        int deviation = mt.voltage * deviate.voltage / 100;
        m.voltage = mt.voltage + random(-1 * deviation, deviation);
    }

    // current
    if (random(prob.current) == prob.current-1){
        int deviation = mt.current * deviate.current / 100;
        m.current = mt.current + random(-1 * deviation, deviation);
    }

    // freq
    if (random(prob.freq) == prob.freq-1){
        int deviation = mt.freq * deviate.freq / 100;
        m.freq = mt.freq + random(-1 * deviation, deviation);
    }

    // pf
    if (random(prob.pf) == prob.pf-1){
        int deviation = mt.pf * deviate.pf / 100;
        m.pf = mt.pf + random(-1 * deviation, deviation);
        if (m.pf > 100) m.pf = 100;
    }
}

void FakeMeterPZ003::updnrg(pz003::metrics& m){
    int64_t t = esp_timer_get_time() >> 10;
    _nrg += mt.power * (t - timecount) / 10;      // find energy for the last time interval in W*ms
    timecount = t;
    mt.power = m.voltage * m.current * m.pf / 100000;     // 100000 = 100 is for pf, 1000 is for decivolts*ma (dw)
    mt.energy += _nrg / 3600000;            // increment W*h counter if we have enough energy consumed so far
    _nrg %= 3600000;
}

// ****  Dummy PZEM004 Implementation  **** //

void DummyPZ003::updateMetrics(){
    pz.update_us = esp_timer_get_time();
    fm.randomize(pz.data);
    fm.updnrg(pz.data);

    pz.data.power = fm.mt.power;
    pz.data.energy = fm.mt.energy;

    if (rx_callback)
        rx_callback(id, nullptr);           // run external call-back function with null data,
                                            // still possible to retrieve metrics from the PZEM obj
}

#endif // ARDUINO
