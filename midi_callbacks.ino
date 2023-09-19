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
 * @file midi_callbacks.ino
 * @author Marcel Licence
 * @date 06.09.2023
 *
 * @brief This file contains the MIDI callbacks
 * please link the functions in z_config.ino (in your MIDI mapping)
 */


/*
 * MIDI callbacks
 */
inline void AppBtn(uint8_t param, uint8_t value)
{
    if (value > 0)
    {
        switch (param)
        {
#ifdef BOARD_ESP32_AUDIO_KIT_AC101

        case 0:
            ac101_setSourceMic();
            break;
        case 1:
            ac101_setSourceLine();
            break;
#endif
        case 2:
            hq_enabled = true;
            break;
        case 3:
            hq_enabled = false;
            break;
        }
    }
}

void AppSetInputGain(uint8_t unused __attribute__((unused)), uint8_t value)
{
    inputGain = log2fromU7(value, -16, 5);
    Status_ValueChangedFloat("InputGain", inputGain);
}

void AppReverb_SetLevel(uint8_t param __attribute__((unused)), uint8_t value)
{
    Reverb_SetLevel(param, floatFromU7(value));
}

void AppTremolo_SetDepth(uint8_t param, uint8_t value)
{
    tremolo.setDepth(floatFromU7(value));
}

void AppVibrato_SetDepth(uint8_t param, uint8_t value)
{
    vibrato.setDepth(floatFromU7(value));
}

void AppVibrato_SetIntensity(uint8_t param, uint8_t value)
{
    vibrato.setIntensity(floatFromU7(value));
}

void App_DelayQ_SetLength(uint8_t unused __attribute__((unused)), uint8_t value)
{
    uint32_t val32 = value;
    val32 *= 189;
    DelayQ_SetLength(unused, val32);
}

void Lfo1_SetSpeed(uint8_t unused, uint8_t value)
{
    float f = value;
    f *= (1.0f / 127.0f);

    float minV = -2;
    float maxV = 3;
    f *= maxV - minV;
    f += minV;
    f = pow(10, f);
    lfo1.setFrequency(f);
    lfo2.setFrequency(f);
}

void PitchShifter_SetSpeed(uint8_t unused, uint8_t value)
{
    float f = value;

#if 1
    f -= 64.0f;
    f /= 12.0f;
    f = pow(2, f);
#else
    f *= (1.0f / 127.0f);

    float minV = -8;
    float maxV = 3;
    f *= maxV - minV;
    f += minV;
    f = pow(2, f);
#endif
    Serial.printf("%s: %f\n", "PitchShifter_SetSpeed", f);

    pitchShifter.setSpeed(-f);
}

void PitchShifter_SetMix(uint8_t unused, uint8_t value)
{
    pitchShifter.setMix(floatFromU7(value));
}

void PitchShifter_SetFeedback(uint8_t unused, uint8_t value)
{
    pitchShifter.setFeedback(floatFromU7(value));
}

