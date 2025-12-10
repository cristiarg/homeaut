
// #include <avr/sleep.h>
// #include <avr/power.h>

#include "timer1_helper.h"

static Timer1Helper timer1;

uint32_t delay_ms = 50;

void example_fire_once_channels_a_b() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleAOnce(350, []() {
        // WORK: do something
        Serial.print("end A ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A ");
        Serial.println(err_code);
    }

    err_code = timer1.scheduleBOnce(800, []() {
        // WORK: do something
        Serial.print("end B ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B ");
        Serial.println(err_code);
    }
}

void example_where_prescaler_calculates_incompatible_value_between_the_two_channels_and_will_not_be_set() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleAOnce(900, []() {
        // WORK: do something
        Serial.print("end A ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A ");
        Serial.println(err_code);
    }

    err_code = timer1.scheduleBOnce(1200, []() {
        // WORK: do something
        Serial.print("end B ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B ");
        Serial.println(err_code);
    }
}

void example_where_delay_is_beyond_the_upper_max_for_timer1() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleBOnce(4195, []() {
        // WORK: do something
        Serial.print("end B ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B ");
        Serial.println(err_code);
    }
}

void example_fire_recurrent_only_channel_A() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleARecurrent(500, 25, []() {
        // WORK: do something
        Serial.print("end A ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A ");
        Serial.println(err_code);
    }
}

void example_fire_recurrent_only_channel_B() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleBRecurrent(938, 8, []() {
        // WORK: do something
        Serial.print("end B ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B ");
        Serial.println(err_code);
    }
}

void example_fire_recurrent_channels_A_B() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleARecurrent(500, 15, []() {
        // WORK: do something
        Serial.print("end A ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A ");
        Serial.println(err_code);
    }

    err_code = timer1.scheduleBRecurrent(938, 8, []() {
        // WORK: do something
        Serial.print("end B ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B ");
        Serial.println(err_code);
    }
}

void example_fire_recurrent_channels_A_B_indenfinitely() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleAIndefinitely(19, []() {
        // WORK: do something
        Serial.print("end A ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A ");
        Serial.println(err_code);
    }

    err_code = timer1.scheduleBIndefinitely(31, []() {
        // WORK: do something
        Serial.print("end B ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B ");
        Serial.println(err_code);
    }
}

/**
 * @todo does not work; nested interrupt only fires correctly once and then - depending on the call to
 * resetB either does not fire at all or fires immediately
 */
void example_fire_one_channel_with_small_delay_nested_into_larger_delay_of_the_other_channel() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta A ");
    Serial.println(millis());

    err_code = timer1.scheduleARecurrent(4000, 5, []() {
        // WORK: do something
        Serial.print("end A ");
        Serial.println(millis());

        const int8_t err_code = timer1.scheduleBOnce(1100, []() {
            // WORK: do something
            Serial.print("  end B ");
            Serial.println(millis());
        });
        if (err_code != Timer1Helper::ER_OK) {
            Serial.print("  ERR: B ");
            Serial.println(err_code);
        }
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A ");
        Serial.println(err_code);
    }
}

void _tandem_schedule_a() {
    int8_t err_code = 0;

    err_code = timer1.scheduleAOnce(1000, []() {
        // WORK: do something
        Serial.print("end A ");
        Serial.println(millis());

        _tandem_schedule_b();
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A ");
        Serial.println(err_code);
    }

}

void _tandem_schedule_b() {
    int8_t err_code = 0;

    err_code = timer1.scheduleBOnce(500, []() {
        // WORK: do something
        Serial.print("end B ");
        Serial.println(millis());

        _tandem_schedule_a();
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B ");
        Serial.println(err_code);
    }
}

void example_tandem() {
    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    _tandem_schedule_a();
}

int _example_very_large_delay_cnt;
void example_very_large_delay_and_stop_from_within_handler_routine() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    _example_very_large_delay_cnt = 0;
    err_code = timer1.scheduleAIndefinitely(143, []() {
        // // WORK: do something
        ++_example_very_large_delay_cnt;
        if (_example_very_large_delay_cnt >= 100) {
            timer1.resetA();
            Serial.print("end at ");
            Serial.println(millis());
        }
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A ");
        Serial.println(err_code);
    }
}

void example_very_large_recurring_delay_without_stop() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    _example_very_large_delay_cnt = 0;
    err_code = timer1.scheduleAIndefinitely(143, []() {
        // // WORK: do something
        ++_example_very_large_delay_cnt;
        if (_example_very_large_delay_cnt == 50) {
            _example_very_large_delay_cnt = 0;
            Serial.print("lap at ");
            Serial.println(millis());
        }
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A ");
        Serial.println(err_code);
    }
}


void setup() {
    Serial.begin(9600);
    Serial.println("init");

    timer1.reset();

    example_fire_once_channels_a_b();
    //example_where_prescaler_calculates_incompatible_value_between_the_two_channels_and_will_not_be_set();
    //example_where_delay_is_beyond_the_upper_max_for_timer1();
    //example_fire_recurrent_only_channel_A();
    //example_fire_recurrent_only_channel_B();
    //example_fire_recurrent_channels_A_B();
    //example_fire_recurrent_channels_A_B_indenfinitely();
    //example_fire_one_channel_with_small_delay_nested_into_larger_delay_of_the_other_channel();
    //example_tandem();
    //example_very_large_delay_and_stop_from_within_handler_routine();
    //example_very_large_recurring_delay_without_stop();
}

void loop() {
    // do normal tasks, timer will fire callbacks asynchronously
}
