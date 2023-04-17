#ifndef INITIATOR_H
#define INITIATOR_H

#include "systemc.h"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/tlm_quantumkeeper.h"

#include <cstdint>
#include "map.h"

// Initiator module generating generic payload transactions
SC_MODULE(Initiator)
{
  // TLM-2 socket, defaults to 32-bits wide, base protocol
  tlm_utils::simple_initiator_socket<Initiator> socket;

  SC_CTOR(Initiator)
  : socket("socket")  // Construct and name socket
  {
    SC_THREAD(thread_process);
    
    // Set the global quantum
    // Please modify this number to test different quantum
    // All initiators will synchronize at this quantum duration
    m_qk.set_global_quantum( sc_time(10, SC_NS) );
    m_qk.reset();
  }

  void thread_process()
  {
    // TLM-2 generic payload transaction, reused across calls to b_transport
    tlm::tlm_generic_payload* trans = new tlm::tlm_generic_payload;
    sc_time delay = sc_time(10, SC_NS);

    // Generate 64 transaction of write and read
    for (int i = 0; i < 64; i++)
    {

      delay = m_qk.get_local_time();
      tlm::tlm_command cmd = tlm::TLM_WRITE_COMMAND; 
      //prepare 4 bytes (uint8_t)
      
      for (int k = 0; k < 3; k++) {
        
        index = a*2-k+1;
        if (index < 0) {
          data[k] = 0;
        } else {
          data[k] = x_input_signal[index];
          // cout << x_input_signal[index] << endl;
        }
      }
      data[3] = 0
      

      // Prepare payload
      trans->set_command( cmd );
      trans->set_address( BASE_TARGET_INPUT_ADDR ); //P2P TLM write to target's address 0
      trans->set_data_ptr( reinterpret_cast<unsigned char*>(&data) );
      trans->set_data_length( 4 );
      trans->set_streaming_width( 4 ); // = data_length to indicate no streaming
      trans->set_byte_enable_ptr( 0 ); // 0 indicates unused
      trans->set_dmi_allowed( false ); // Mandatory initial value
      trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value

      socket->b_transport( *trans, delay );  // Blocking transport call with current local time

      // Initiator obliged to check response status and delay
      if ( trans->is_response_error() )
        SC_REPORT_ERROR("TLM-2", "Response error from b_transport");

      // Increment local time with delay returned from b_transport()
      m_qk.inc( delay );
      // Check if synchronize is necessary
      if (m_qk.need_sync()) m_qk.sync();

      //Read back results
      cmd = tlm::TLM_READ_COMMAND; 

      // Prepare payload
      trans->set_command( cmd );
      trans->set_address( BASE_TARGET_OUTPUT_ADDR ); //P2P TLM read from target's address 4
      trans->set_data_ptr( reinterpret_cast<unsigned char*>(&result) );
      trans->set_data_length( 4 );
      trans->set_streaming_width( 4 ); // = data_length to indicate no streaming
      trans->set_byte_enable_ptr( 0 ); // 0 indicates unused
      trans->set_dmi_allowed( false ); // Mandatory initial value
      trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value

      socket->b_transport( *trans, delay );  // Blocking transport call

      // Initiator obliged to check response status and delay
      if ( trans->is_response_error() )
        SC_REPORT_ERROR("TLM-2", "Response error from b_transport");

      cout << "trans = " << (cmd ? 'W' : 'R')
           << ", result = " << result << " at time " << sc_time_stamp()
           << " delay = " << delay << endl;

      // Increment local time with delay returned from b_transport()
      m_qk.inc( delay );
      // Check if synchronize is necessary
      if (m_qk.need_sync()) m_qk.sync();
    }
  }

  tlm_utils::tlm_quantumkeeper m_qk; // Quantum keeper for temporal decoupling

  // Internal data buffer used by initiator with generic payload
  sc_ufixed_fast<4, 4> data[4];
  sc_ufixed_fast<16, 16> result;
};

#endif
