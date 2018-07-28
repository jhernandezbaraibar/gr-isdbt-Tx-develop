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
#include "hierarchical_divisor_impl.h"

namespace gr {
  namespace isdbt {

    hierarchical_divisor::sptr
    hierarchical_divisor::make()
    {
      // TRABAJAR EN GENERALIZAR ESTE BLOQUE
      //ESTA SOLO PARA MONTECARLO . TS
      return gnuradio::get_initial_sptr
        (new hierarchical_divisor_impl());
    }

    /*
     * The private constructor
     */
    hierarchical_divisor_impl::hierarchical_divisor_impl()
      : gr::block("hierarchical_divisor",
              gr::io_signature::make(1, 1, 2176*204*sizeof(unsigned char)),
              gr::io_signature::make3(3, 3, 118*204*sizeof(unsigned char),
                                            712*204*sizeof(unsigned char),
                                            712*204*sizeof(unsigned char))
                                      )
    {}

    /*
     * Our virtual destructor.
     */
    hierarchical_divisor_impl::~hierarchical_divisor_impl()
    {
    }

    void
    hierarchical_divisor_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = noutput_items;
    }

    int
    hierarchical_divisor_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const unsigned char *in = (const unsigned char *) input_items[0];
      unsigned char *out0 = (unsigned char *) output_items[0];
      unsigned char *out1 = (unsigned char *) output_items[1];
      unsigned char *out2 = (unsigned char *) output_items[2];

      unsigned char tsp_info_byte; //buffer to read layer information
      int layer_flag = 0;
      int tsp_a = 0;
      int tsp_b = 0;
      int tsp_c = 0;
      int def = 0;

      for (int output = 0; output < noutput_items; output++)
      {
        //printf("NUEVO OUTPUT ITEM\n");

        for (int k=0; k<total_tsp_in_multiplex ; k++)
        {
          //Read first TSP from input
          tsp_info_byte = in[output*total_tsp_in_multiplex*tsp_len + k*tsp_len + 188 + 1];
          layer_flag = tsp_info_byte >>= 4;
          
          switch (layer_flag)
          {
            case 1:
            {
              //Copy TSP to output A
              memcpy(out0+ output*layer_a_tsp*204 + tsp_len*tsp_a, in + k*tsp_len + output*2176*204, tsp_len);
              tsp_a++;
              break;
            }
            case 2:
            {
              //Copy TSP to output B
              memcpy(out1+ output*layer_b_tsp*tsp_len + tsp_len*tsp_b, in + k*tsp_len + output*total_tsp_in_multiplex*tsp_len, tsp_len);
              tsp_b++;
              break;
            }
            case 3:
            {
              //Copy TSP to output C
              memcpy(out2+ output*layer_c_tsp*tsp_len + tsp_len*tsp_c, in + k*tsp_len + output*total_tsp_in_multiplex*tsp_len, tsp_len);
              tsp_c++;
              break;
            }
            default:
            {
              //printf("Layer X, total: %i \n", def);
              def++;
              break;
            }
          }
          //Goto test new TSP
        }
        tsp_a = 0;
        tsp_b = 0;
        tsp_c = 0;
        def = 0;
      }
      // Tell runtime system how many input items we consumed on
      // each input stream.
      this->consume(0, noutput_items);
      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace isdbt */
} /* namespace gr */

