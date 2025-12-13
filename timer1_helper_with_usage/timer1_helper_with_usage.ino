#include "timer1_helper.h"

static Timer1Helper timer1;

void example_fire_once_channels_a_b() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    const uint32_t _delay_ms_A = 350;
    err_code = timer1.scheduleAOnce(_delay_ms_A, [](bool) {
        Serial.print("end A at ");
        Serial.print(millis());
        Serial.print(" after ");
        Serial.print(_delay_ms_A);
        Serial.println(" ms");
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A code ");
        Serial.println(err_code);
    }

    const uint32_t _delay_ms_B = 800;
    err_code = timer1.scheduleBOnce(_delay_ms_B, [](bool) {
        Serial.print("end B at ");
        Serial.print(millis());
        Serial.print(" after ");
        Serial.print(_delay_ms_B);
        Serial.println(" ms");
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B code ");
        Serial.println(err_code);
    }
}

void example_error_prescaler_calculates_incompatible_value_between_the_two_channels_and_will_not_be_set() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleAOnce(900, [](bool) {
        Serial.print("end A");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A code ");
        Serial.println(err_code);
    }

    err_code = timer1.scheduleBOnce(1200, [](bool) {
        Serial.println("B - should not be executed");
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B code ");
        Serial.println(err_code);
    }
}

void example_error_delay_is_beyond_the_upper_max_for_timer1() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleBOnce(4195, [](bool) {
        Serial.println("B - should not be executed");
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B code ");
        Serial.println(err_code);
    }
}

void example_error_invalid_recurrence_value() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleBRecurrent(4195, -10, [](bool) {
        Serial.println("B - should not be executed");
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B code ");
        Serial.println(err_code);
    }
}

void example_error_missing_callback_function() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleBOnce(4195, nullptr);
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B code ");
        Serial.println(err_code);
    }
}

void example_fire_recurrent_only_channel_A() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    const uint32_t _delay_ms_A = 500;
    err_code = timer1.scheduleARecurrent(_delay_ms_A, 25, [](bool) {
        Serial.print("iter A at ");
        Serial.print(millis());
        Serial.print(" after ");
        Serial.print(_delay_ms_A);
        Serial.println(" ms");
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A code ");
        Serial.println(err_code);
    }
}

int _fire_recurrent_only_channel_A_and_stop_early_cnt;
void example_fire_recurrent_only_channel_A_and_stop_early() {
    int8_t err_code = 0;

    timer1.reset(); // clean before
    _fire_recurrent_only_channel_A_and_stop_early_cnt = 0;

    Serial.print("sta ");
    Serial.println(millis());

    const uint32_t _delay_ms_A = 500;
    err_code = timer1.scheduleARecurrent(_delay_ms_A, 127, [](bool) {
        Serial.print("iter A at ");
        Serial.print(millis());
        Serial.print(" after ");
        Serial.print(_delay_ms_A);
        Serial.println(" ms");

        ++_fire_recurrent_only_channel_A_and_stop_early_cnt;
        if (_fire_recurrent_only_channel_A_and_stop_early_cnt == 5) {
            const unsigned long _now_ms = millis();

            timer1.resetA();

            Serial.print("end A at ");
            Serial.print(_now_ms);
            Serial.print(" after [ ");
            Serial.print(_fire_recurrent_only_channel_A_and_stop_early_cnt);
            Serial.print(" (iters) * ");
            Serial.print(_delay_ms_A);
            Serial.println(" (millis) ]");
        }
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A code ");
        Serial.println(err_code);
    }
}

void example_fire_recurrent_only_channel_B() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    const uint32_t _delay_ms_B = 200;
    const int16_t _recurrence_B = 130;
    err_code = timer1.scheduleBRecurrent(_delay_ms_B, _recurrence_B, [](bool) {
        Serial.print("iter B at ");
        Serial.print(millis());
        Serial.print(" after ");
        Serial.print(_delay_ms_B);
        Serial.println(" ms");
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B code ");
        Serial.println(err_code);
    }
}

int _fire_recurrent_only_channel_B_and_stop_early_cnt;
void example_fire_recurrent_only_channel_B_and_stop_early() {
    int8_t err_code = 0;

    timer1.reset(); // clean before
    _fire_recurrent_only_channel_B_and_stop_early_cnt = 0;

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleBRecurrent(30, 127, [](bool) {
        Serial.print("iter B ");
        Serial.println(millis());
        ++_fire_recurrent_only_channel_B_and_stop_early_cnt;
        if (_fire_recurrent_only_channel_B_and_stop_early_cnt == 100) {
            timer1.resetB();
            Serial.print("end B at ");
            Serial.print(millis());
            Serial.print(" after ");
            Serial.print(_fire_recurrent_only_channel_B_and_stop_early_cnt);
            Serial.println(" iters");
        }
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B code ");
        Serial.println(err_code);
    }
}

void example_fire_recurrent_channels_A_B() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleARecurrent(500, 15, [](bool) {
        Serial.print("end A ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A code ");
        Serial.println(err_code);
    }

    err_code = timer1.scheduleBRecurrent(938, 8, [](bool) {
        Serial.print("end B ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B code ");
        Serial.println(err_code);
    }
}

void example_fire_recurrent_channels_A_B_indenfinitely() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    err_code = timer1.scheduleAIndefinitely(19, [](bool) {
        Serial.print("end A ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A code ");
        Serial.println(err_code);
    }

    err_code = timer1.scheduleBIndefinitely(31, [](bool) {
        Serial.print("end B ");
        Serial.println(millis());
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B code ");
        Serial.println(err_code);
    }
}

int8_t _fire_one_channel_with_small_delay_nested_into_larger_delay_of_the_other_channel_iter_A;
void example_fire_one_channel_with_small_delay_nested_into_larger_delay_of_the_other_channel() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta A ");
    Serial.println(millis());

    const uint32_t _delay_ms_A = 3000;
    const int16_t _recurrence_A = 5;
    _fire_one_channel_with_small_delay_nested_into_larger_delay_of_the_other_channel_iter_A = 0;
    err_code = timer1.scheduleARecurrent(_delay_ms_A, _recurrence_A, [](bool) {
        ++_fire_one_channel_with_small_delay_nested_into_larger_delay_of_the_other_channel_iter_A;
        Serial.print("iter A (");
        Serial.print(_fire_one_channel_with_small_delay_nested_into_larger_delay_of_the_other_channel_iter_A);
        Serial.print("/");
        Serial.print(_recurrence_A);
        Serial.print(") at ");
        Serial.println(millis());

        const uint32_t _delay_ms_B = 1100;
        const int8_t err_code = timer1.scheduleBOnce(_delay_ms_B, [](bool) {
            Serial.print("  fire B at ");
            Serial.print(millis());
            Serial.print(" after +");
            Serial.print(_delay_ms_B);
            Serial.println(" ms");
        });
        if (err_code != Timer1Helper::ER_OK) {
            Serial.print("  ERR: B code ");
            Serial.println(err_code);
        }
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A code ");
        Serial.println(err_code);
    }
}

volatile uint32_t _tandem_schedule_A_rnd;
void _tandem_schedule_A() {
    int8_t err_code = 0;

    _tandem_schedule_A_rnd = (uint32_t)random(265, 1046);
    err_code = timer1.scheduleAOnce(_tandem_schedule_A_rnd, [](bool) {
        // save needed values ..
        const unsigned long _now_ms = millis();
        // .. and immediately re-schedule the other handler ..
        _tandem_schedule_B();
        // .. then do actual work asynchronously
        Serial.print("end A @");
        Serial.println(_now_ms);
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A code ");
        Serial.println(err_code);
    }
    Serial.print("sch A @");
    Serial.println((millis() + _tandem_schedule_A_rnd));
}

volatile uint32_t _tandem_schedule_B_rnd;
void _tandem_schedule_B() {
    int8_t err_code = 0;

    _tandem_schedule_B_rnd = (uint32_t)random(265, 1046);
    err_code = timer1.scheduleBOnce(_tandem_schedule_B_rnd, [](bool) {
        // save needed values ..
        const unsigned long _now_ms = millis();
        // .. and immediately schedule the other handler ..
        _tandem_schedule_A();
        // .. then do actual work asynchronously
        Serial.print("end B @");
        Serial.println(_now_ms);
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: B code ");
        Serial.println(err_code);
    }
    Serial.print("sch B @");
    Serial.println((millis() + _tandem_schedule_B_rnd));
}

void example_tandem() {
    timer1.reset(); // clean before

    _tandem_schedule_A();
}

int _example_very_large_delay_cnt;
void example_very_large_delay_and_stop_from_within_handler_routine() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    const uint32_t delay_ms = 143;
    _example_very_large_delay_cnt = 0;
    err_code = timer1.scheduleAIndefinitely(delay_ms, [](bool) {
        ++_example_very_large_delay_cnt;
        if (_example_very_large_delay_cnt >= 100) {
            timer1.resetA();

            Serial.print("end at ");
            Serial.print(millis());
            Serial.print(" after [ ");
            Serial.print(_example_very_large_delay_cnt);
            Serial.print(" (iters) * ");
            Serial.print(delay_ms);
            Serial.println(" (millis) ]");
        }
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A code ");
        Serial.println(err_code);
    }
}

void example_very_large_recurring_delay_without_stop() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    Serial.print("sta ");
    Serial.println(millis());

    _example_very_large_delay_cnt = 0;
    err_code = timer1.scheduleAIndefinitely(143, [](bool) {
        ++_example_very_large_delay_cnt;
        if (_example_very_large_delay_cnt == 50) {
            _example_very_large_delay_cnt = 0;
            Serial.print("lap at ");
            Serial.println(millis());
        }
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A code ");
        Serial.println(err_code);
    }
}

int _schedule_with_a_function_cnt;
void _schedule_with_a_function(bool isDone) {
    ++_schedule_with_a_function_cnt;
    if (isDone) {
        Serial.print("end at ");
        Serial.print(millis());
        Serial.print(" after ");
        Serial.print(_schedule_with_a_function_cnt);
        Serial.println(" iterations");
    }
}

void example_schedule_with_a_function() {
    int8_t err_code = 0;

    timer1.reset(); // clean before

    _schedule_with_a_function_cnt = 0;
    Serial.print("sta ");
    Serial.println(millis());
    err_code = timer1.scheduleBRecurrent(350, 5, &_schedule_with_a_function);
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A code ");
        Serial.println(err_code);
    }
}

volatile uint32_t _self_tandem_schedule_A_rnd;
volatile uint32_t _self_tandem_iter = 0;
void example_self_tandem() {
    int8_t err_code = 0;

    ++_self_tandem_iter;
    _self_tandem_schedule_A_rnd = (uint32_t)random(1050, 3000);
    //timer1.resetA();
    err_code = timer1.scheduleAOnce(_self_tandem_schedule_A_rnd, [](bool) {
        // save needed values ..
        const unsigned long _now_ms = millis();
        const uint32_t _iter = _self_tandem_iter;
        // .. and immediately re-schedule ..
        example_self_tandem();
        // .. then do actual work asynchronously
        Serial.print("end A ");
        Serial.print(_iter);
        Serial.print("@");
        Serial.println(_now_ms);
    });
    if (err_code != Timer1Helper::ER_OK) {
        Serial.print("ERR: A code ");
        Serial.println(err_code);
    }
    Serial.print("sch A ");
    Serial.print(_self_tandem_iter);
    Serial.print("@");
    Serial.println((millis() + _self_tandem_schedule_A_rnd));
}

void setup() {
    Serial.begin(9600);
    Serial.println("init");

    randomSeed(analogRead(0));

    timer1.reset();

    //example_fire_once_channels_a_b();
    //example_error_prescaler_calculates_incompatible_value_between_the_two_channels_and_will_not_be_set();
    //example_error_delay_is_beyond_the_upper_max_for_timer1();
    //example_error_invalid_recurrence_value();
    //example_error_missing_callback_function();
    //example_fire_recurrent_only_channel_A();
    //example_fire_recurrent_only_channel_A_and_stop_early();
    //example_fire_recurrent_only_channel_B();
    //example_fire_recurrent_only_channel_B_and_stop_early();
    //example_fire_recurrent_channels_A_B();
    //example_fire_recurrent_channels_A_B_indenfinitely();
    //example_fire_one_channel_with_small_delay_nested_into_larger_delay_of_the_other_channel();
    //example_tandem();
    //example_very_large_delay_and_stop_from_within_handler_routine();
    //example_very_large_recurring_delay_without_stop();
    //example_schedule_with_a_function();
    //example_self_tandem();
}

void loop() {
    // do normal tasks, timer will fire callbacks asynchronously
}
