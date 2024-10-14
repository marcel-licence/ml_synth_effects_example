/*
 * Copyright (c) 2023 Marcel Licence
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Dieses Programm ist Freie Software: Sie können es unter den Bedingungen
 * der GNU General Public License, wie von der Free Software Foundation,
 * Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
 * veröffentlichten Version, weiter verteilen und/oder modifizieren.
 *
 * Dieses Programm wird in der Hoffnung bereitgestellt, dass es nützlich sein wird, jedoch
 * OHNE JEDE GEWÄHR,; sogar ohne die implizite
 * Gewähr der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
 * Siehe die GNU General Public License für weitere Einzelheiten.
 *
 * Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 * Programm erhalten haben. Wenn nicht, siehe <https://www.gnu.org/licenses/>.
 */

/**
 * @file ml_synth_effects_example.ino
 * @author Marcel Licence
 * @date 06.09.2023
 *
 * @brief   This is the main project file of the effects project
 *          It should be compatible with ESP32 (other platforms may follow soon: ESP8266, Seedstudio XIAO, PRJC Teensy 4.1, Electrosmith Daisy Seed, Raspberry Pi Pico, STM32F4)
 *
 * @see little demo: https://youtu.be/hqK_U22Jha8
 */


#include "config.h"


#include <Arduino.h>


/*
 * Library can be found on https://github.com/marcel-licence/ML_SynthTools
 * and you might need https://github.com/marcel-licence/ML_SynthTools_Lib too
 */
#include <ml_delay_q.h>
#include <ml_lfo.h>
#include <ml_phaser.h>
#include <ml_pitch_shifter.h>
#include <ml_reverb.h>
#include <ml_tremolo.h>
#include <ml_types.h>
#include <ml_utils.h>
#include <ml_vibrato.h>


/* centralized modules */
#define ML_SYNTH_INLINE_DECLARATION
#include <ml_inline.h>
#undef ML_SYNTH_INLINE_DECLARATION


float lfo1_buffer[SAMPLE_BUFFER_SIZE];
ML_LFO lfo1(SAMPLE_RATE, lfo1_buffer, SAMPLE_BUFFER_SIZE);

float lfo2_buffer[SAMPLE_BUFFER_SIZE];
ML_LFO lfo2(SAMPLE_RATE, lfo2_buffer, SAMPLE_BUFFER_SIZE);

ML_Tremolo tremolo(SAMPLE_RATE);

ML_Vibrato vibrato(SAMPLE_RATE);

ML_PitchShifter pitchShifter(SAMPLE_RATE);


/**
    @brief This function contains the setup routines.
 */
void setup()
{
    Serial.begin(SERIAL_BAUDRATE);
    Serial.print(F("ml_synth_effects_example\n"));

    /*
     * this code runs once
     */
#ifdef MIDI_USB_ENABLED
    Midi_Usb_Setup();
#endif

#ifdef BLINK_LED_PIN
    Blink_Setup();
    Blink_Fast(3);
#endif

    delay(250);

    Audio_Setup();


    Serial.println();
    delay(250);

    Serial.printf("Loading data\n");


    Serial.printf("Audio Effects Example\n");

    static float phaser_buffer[PHASER_BUFFER_SIZE];
    Phaser_Init(phaser_buffer, PHASER_BUFFER_SIZE);

    lfo1.setPhase(0);
    lfo2.setPhase(M_PI / 2.0f); /* like sine and cosine */

    Midi_Setup();

    /*
     * Initialize reverb
     * The buffer shall be static to ensure that
     * the memory will be exclusive available for the reverb module
     */
    static float *revBuffer = (float *)malloc(sizeof(float) * REV_BUFF_SIZE);
    Reverb_Setup(revBuffer);

    /*
     * Prepare a buffer which can be used for the delay
     */
    static int16_t *delBuffer1 = (int16_t *)malloc(sizeof(int16_t) * MAX_DELAY_Q);
    static int16_t *delBuffer2 = (int16_t *)malloc(sizeof(int16_t) * MAX_DELAY_Q);
    DelayQ_Init2(delBuffer1, delBuffer2, MAX_DELAY_Q);

    Serial.printf("setup done!\n");
}


/**
    @brief This function will be called every second.
        In case of an overload the function will not called so often
  */
void loop_1Hz()
{
#ifdef BLINK_LED_PIN
    Blink_Process();
#endif
}

bool hq_enabled = true;
float inputGain = 1.0f;

/**
    @brief This function contains the mainloop
 */
void loop()
{
    static int loop_cnt_1hz = 0; /*!< counter to allow 1Hz loop cycle */

    loop_cnt_1hz += SAMPLE_BUFFER_SIZE;

    if (loop_cnt_1hz >= SAMPLE_RATE)
    {
        loop_cnt_1hz -= SAMPLE_RATE;
        loop_1Hz();
    }

    /*
     * MIDI processing
     */
    Midi_Process();

#ifdef MIDI_VIA_USB_ENABLED
    UsbMidi_ProcessSync();
#endif

#ifdef MIDI_STREAM_PLAYER_ENABLED
    MidiStreamPlayer_Tick(SAMPLE_BUFFER_SIZE);
#endif

#ifdef MIDI_BLE_ENABLED
    midi_ble_loop();
#endif

#ifdef USB_HOST_ENABLED
    Usb_Host_Midi_loop();
#endif

#ifdef MIDI_USB_ENABLED
    Midi_Usb_Loop();
#endif

    /*
     * And finally the audio stuff
     */
    Q1_14 left[SAMPLE_BUFFER_SIZE], right[SAMPLE_BUFFER_SIZE];

    memset(left, 0, sizeof(left));
    memset(right, 0, sizeof(right));


    Audio_Input(left, right);

    mul(left, inputGain, left, SAMPLE_BUFFER_SIZE);
    mul(right, inputGain, right, SAMPLE_BUFFER_SIZE);

    /*
     * usually the synthesizer processing would be placed here
     */
    float mono[SAMPLE_BUFFER_SIZE];

    mixStereoToMono(left, right, mono, SAMPLE_BUFFER_SIZE);

    Reverb_Process(mono, SAMPLE_BUFFER_SIZE);


    lfo2.Process(SAMPLE_BUFFER_SIZE);

#ifdef LFO1_MODULATED_BY_LFO1
    ScaleLfo(lfo2_buffer, lfo2_buffer_scale, SAMPLE_BUFFER_SIZE, 0.1, 10);
    lfo1.Process(lfo2_buffer_scale, SAMPLE_BUFFER_SIZE);
#else
    lfo1.Process(SAMPLE_BUFFER_SIZE);
#endif

    if (hq_enabled)
    {
        Phaser_ProcessHQ(mono, lfo1_buffer, mono, SAMPLE_BUFFER_SIZE);
    }
    else
    {
        Phaser_Process(mono, lfo1_buffer, mono, SAMPLE_BUFFER_SIZE);
    }


    if (hq_enabled)
    {
        vibrato.ProcessHQ(mono, lfo1_buffer, mono, SAMPLE_BUFFER_SIZE);
    }
    else
    {
        vibrato.Process(mono, lfo1_buffer, mono, SAMPLE_BUFFER_SIZE);
    }

    if (hq_enabled)
    {
        pitchShifter.ProcessHQ(mono, mono, SAMPLE_BUFFER_SIZE);
    }
    else
    {
        pitchShifter.Process(mono, mono, SAMPLE_BUFFER_SIZE);
    }

    float f_l[SAMPLE_BUFFER_SIZE], f_r[SAMPLE_BUFFER_SIZE];
    tremolo.Process(mono, mono, lfo1_buffer, f_l, f_r, SAMPLE_BUFFER_SIZE);

    for (int n = 0; n < SAMPLE_BUFFER_SIZE; n++)
    {
        left[n].s16 = f_l[n] * 16384;
        right[n].s16 = f_r[n] * 16384;
    }

    /*
     * post process delay
     */
    DelayQ_Process_Buff(&left[0].s16, &right[0].s16, &left[0].s16, &right[0].s16, SAMPLE_BUFFER_SIZE);

    /*
     * Output the audio
     */
    Audio_Output(left, right);
}

