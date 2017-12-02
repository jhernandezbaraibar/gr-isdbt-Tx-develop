/* -*- c++ -*- */
/*  
 * Copyright 2015, 2016, 2017
 *   Federico "Larroca" La Rocca <flarroca@fing.edu.uy>
 *   Pablo Belzarena 
 *   Gabriel Gomez Sena 
 *   Pablo Flores Guridi 
 *   Victor Gonzalez Barbone
 * 
 *   Instituto de Ingenieria Electrica, Facultad de Ingenieria,
 *   Universidad de la Republica, Uruguay.
 *  
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *  
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 * 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "ofdm_frame_structure_impl.h"

namespace gr {
  namespace isdbt {

    ofdm_frame_structure::sptr
    ofdm_frame_structure::make(int mode)
    {
      return gnuradio::get_initial_sptr
        (new ofdm_frame_structure_impl(mode));
    }

    /*
     * The private constructor
     */
    ofdm_frame_structure_impl::ofdm_frame_structure_impl(int mode)
      : gr::block("ofdm_frame_structure",
              gr::io_signature::make(1, 1, sizeof(gr_complex) * 13 * 96 * ((int)pow(2.0,mode-1))),
              gr::io_signature::make(1, 1, sizeof(gr_complex) * ((int)pow(2.0,10 + mode)))
              )
    {
      Frame_counter = 0;      /* Counts frames*/
      d_symbol_counter = 0;  /* Counts symbols across frames */
      d_carrier_pos = 0;    /* 4 cyclic counter, scattered pilot rotation reference*/
      TMCCindex = 0;        /* TMCC word position counter, accross block */
      SPindex = 0;          /* Scattered pilot counter, accross segment */
      d_mode = mode;        /* Transmission Mode */
      sp_keyword = 0b00000000000; /* SP keyword default*/
      TMCCword.set();
      TMCC_sync_word = 0b0011010111101110;
    }

    /*
     * Our virtual destructor.
     */
    ofdm_frame_structure_impl::~ofdm_frame_structure_impl()
    {
    }

    
    bitset<11> 
    ofdm_frame_structure_impl::ObtainStartingWord(int SegmentNumber, int d_mode)
    {
      switch(d_mode) // 
      {
      case 1 :  
        sp_segment_keywords[0] = 0b11001000010;
        sp_segment_keywords[1] = 0b00101111010;
        sp_segment_keywords[2] = 0b00010000100;
        sp_segment_keywords[3] = 0b11011100101;
        sp_segment_keywords[4] = 0b10010100000;
        sp_segment_keywords[5] = 0b01000101110;
        sp_segment_keywords[6] = 0b11110110000;
        sp_segment_keywords[7] = 0b01101011110;
        sp_segment_keywords[8] = 0b00001011000;
        sp_segment_keywords[9] = 0b11011001111;
        sp_segment_keywords[10]= 0b10100100111;
        sp_segment_keywords[11]= 0b11111111111;
        sp_segment_keywords[12]= 0b01110001001;
        break;       
      case 2 : 
        sp_segment_keywords[0] = 0b11011100101;
        sp_segment_keywords[1] = 0b00010011100;
        sp_segment_keywords[2] = 0b00000100100;
        sp_segment_keywords[3] = 0b10010100000;
        sp_segment_keywords[4] = 0b00100011001;
        sp_segment_keywords[5] = 0b11001000010;
        sp_segment_keywords[6] = 0b01100111001;
        sp_segment_keywords[7] = 0b11011100101;
        sp_segment_keywords[8] = 0b11100110110;
        sp_segment_keywords[9] = 0b01101011110;
        sp_segment_keywords[10]= 0b00101010001;
        sp_segment_keywords[11]= 0b11111111111;
        sp_segment_keywords[12]= 0b00100001011;
        break;
      case 3 :
        sp_segment_keywords[0] = 0b10010100000;
        sp_segment_keywords[1] = 0b11100110110;
        sp_segment_keywords[2] = 0b11100111101;
        sp_segment_keywords[3] = 0b00100011001;
        sp_segment_keywords[4] = 0b01101010011;
        sp_segment_keywords[5] = 0b01110001001;
        sp_segment_keywords[6] = 0b10111010010;
        sp_segment_keywords[7] = 0b10010100000;
        sp_segment_keywords[8] = 0b01100010010;
        sp_segment_keywords[9] = 0b11011100101;
        sp_segment_keywords[10] = 0b11110100101;
        sp_segment_keywords[11] = 0b11111111111;
        sp_segment_keywords[12] = 0b00010011100;
        break;
      default:
        printf("Error: invalid Transmission mode \n");
        break;
      }
      bitset<11> Keyword = (sp_segment_keywords[SegmentNumber]);
      return Keyword;
    }

    /*
    Writes corresponding Scattered Pilot into symbol
    */
    gr_complex 
    ofdm_frame_structure_impl::write_SP(int SPindex, int d_mode, int SegmentNumber)
    {
      if (SPindex == 0)
      {
        sp_keyword = this->ObtainStartingWord(SegmentNumber, d_mode);
      }
      if (sp_keyword.test(SPindex)) /*Return bit value in keyword for SPindex*/
      {
        return std::complex<float>(-4.0/3.0, 0);   
      } else {
        return std::complex<float>(4.0/3.0, 0);
      }
    }

    /*
    Writes bits of the TMCC into symbol
    */
    gr_complex 
    ofdm_frame_structure_impl::write_TMCC(int SymbolNumber, int Frame_counter)
    {
      printf("Dentro de write TMCC, SymbolNumber: %d \n", SymbolNumber);
      //Test TMCC word for writing bit, returns symbol mapped into DBPSK
      bool current_bit, previous_bit;
      //First bit, only decide on it
      if (SymbolNumber == 0)
      {
        //Assign b0, check for SP0 value
        gr_complex sp0 = this->write_SP(SPindex, d_mode, 0);
        //Todo: Reemplazar el 0 por Segment Number
        if(sp0.real() < 0)
        {
          TMCCword.set(0);
        } else {
          TMCCword.reset(0);
        }
        //Assign b1-b16
        bool EsPar = ((Frame_counter % 2) == 0);
        for (int i = 0; i < 16; i++)
        {
          if (EsPar)
          {
            if (TMCC_sync_word.test(i))
            {
              TMCCword.set(i+1);
            } else {
              TMCCword.reset(i+1);
            }
          }
          else
          {
            if (TMCC_sync_word.test(i))
            {
              TMCCword.reset(i+1);
            } else {
              TMCCword.set(i+1);
            }
          }
        }
        //Syncronism signal
        //Assign b17-b19
        //Segment type identifier
        //Assign b20-b121
        //TMCC information bits
        //Assign b122-b203
        // Return bit0
        return sp0;
      }
      //General case, decision based on current bit and previous bit
      current_bit = TMCCword.test(SymbolNumber);
      previous_bit = TMCCword.test(SymbolNumber - 1);
      if (!((current_bit & previous_bit) || (!current_bit & !previous_bit)))
      {
        // Send 1
        return std::complex<float>(-4.0/3.0, 0);   
      } else {
        // Send 0
        return std::complex<float>(4.0/3.0, 0);
      }
    }



    void
    ofdm_frame_structure_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = noutput_items;
    }

    int
    ofdm_frame_structure_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
    const gr_complex *in = (const gr_complex *) input_items[0];
    gr_complex *out = (gr_complex *) output_items[0];
    int k = 0;
    for (int i = 0; i < noutput_items ; i++) 
    {
        if (d_symbol_counter == 0)
        {
          //printf("Estoy en el primer simbolo del cuadro\n");
          Frame_counter++;
          TMCCindex = 0;
        }
      switch (d_mode)
        {
        case 1:
          /* Segment 0*/
          for (int j = 0; j < 96; j++) 
          {
            if ((j % 12) == (3*d_carrier_pos))
            {
              /*Scattered Pilot*/
              out[108*6+j] = this->write_SP(SPindex, d_mode, 0 /*Segment number*/);
              SPindex++;
            } else if (j == 49) {
              /* TMCC */
              printf("Antes de llamar a writeTMCC, d_symbol_counter %d\n", d_symbol_counter);
              out[108*6+j] = this->write_TMCC(d_symbol_counter, Frame_counter);
              TMCCindex++;
              printf("TMCCindex: %d \n", TMCCindex);
              printf("out[TMCC]: (%f,%f) \n", out[108*6+j].real(), out[108*6+j].imag());
            } else if ((j == 35) || (j == 79)) {
              /* AC1 or AC2 */
              out[108*6+j] = std::complex<double>(0, 0);
            } else {
              /* Fill with data*/
              out[108*6+j] = in[k];
              k++;
            }
          }
          SPindex = 0;
          break;
        case 2:
          printf("Mode 2 \n");
          for (int j = 0; j < 4096; j++)
          {
          out[j] = std::complex<double>(100, 200);
          }
          break;
        case 3:
          printf("Mode 3 \n");
          for (int j = 0; j < 8192; j++)
          {
          out[j] = std::complex<double>(100, 200);
          }
          break;
        default:
          printf("Error: incorrect mode \n");
          break; 
        } 
        d_symbol_counter++;  
        d_carrier_pos = (d_symbol_counter % 4);
        d_symbol_counter = (d_symbol_counter % 204);
      }
      this->consume(0, noutput_items);
      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace isdbt */
} /* namespace gr */

