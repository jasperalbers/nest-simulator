/*
 *  sirs_neuron.h
 *
 *  This file is part of NEST.
 *
 *  Copyright (C) 2004 The NEST Initiative
 *
 *  NEST is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NEST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NEST.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SIRS_NEURON_H
#define SIRS_NEURON_H

// C++ includes:
#include <cmath>
#include <limits>

// Includes from libnestutil:
#include "dict_util.h"
#include "numerics.h"

// Includes from nestkernel:
#include "archiving_node.h"
#include "connection.h"
#include "event.h"
#include "event_delivery_manager_impl.h"
#include "exceptions.h"
#include "kernel_manager.h"
#include "nest_timeconverter.h"
#include "nest_types.h"
#include "random_generators.h"
#include "recordables_map.h"
#include "ring_buffer.h"
#include "universal_data_logger.h"
#include "universal_data_logger_impl.h"

// Includes from sli:
#include "dict.h"
#include "dictutils.h"
#include "doubledatum.h"
#include "integerdatum.h"

namespace nest
{
/**
 * SIRS neuron with linear or sigmoidal gain function.
 *
 * This class is a base class that needs to be instantiated with a gain
 * function.
 *
 * @note
 * This neuron has a special use for spike events to convey the
 * sirs state of the neuron to the target. The neuron model
 * only sends a spike if a transition of its state occurs. If the
 * state makes an up-transition it sends a spike with multiplicity 2,
 * if a down-transition occurs, it sends a spike with multiplicity 1.
 * The decoding scheme relies on the feature that spikes with multiplicity
 * larger than 1 are delivered consecutively, also in a parallel setting.
 * The creation of double connections between sirs neurons will
 * destroy the decoding scheme, as this effectively duplicates
 * every event. Using random connection routines it is therefore
 * advisable to set the property 'allow_multapses' to false.
 * The neuron accepts several sources of currents, e.g. from a
 * noise_generator.
 *
 * @see ginzburg_neuron, mccullogh_pitts_neuron, erfc_neuron
 */
class sirs_neuron : public ArchivingNode
{

public:
  sirs_neuron();
  sirs_neuron( const sirs_neuron& );

  /**
   * Import sets of overloaded virtual functions.
   * @see Technical Issues / Virtual Functions: Overriding, Overloading, and
   * Hiding
   */
  using Node::handle;
  using Node::handles_test_event;
  using Node::receives_signal;
  using Node::sends_signal;

  port send_test_event( Node&, rport, synindex, bool );

  void handle( SpikeEvent& );
  void handle( CurrentEvent& );
  void handle( DataLoggingRequest& );

  port handles_test_event( SpikeEvent&, rport );
  port handles_test_event( CurrentEvent&, rport );
  port handles_test_event( DataLoggingRequest&, rport );

  SignalType sends_signal() const;
  SignalType receives_signal() const;

  void get_status( DictionaryDatum& ) const;
  void set_status( const DictionaryDatum& );

  void calibrate_time( const TimeConverter& tc );


private:
  void init_buffers_();
  void pre_run_hook();

  // gain function functor
  // must have an double operator(double) defined

  void update( Time const&, const long, const long );

  // The next two classes need to be friends to access the State_ class/member
  friend class RecordablesMap< sirs_neuron >;
  friend class UniversalDataLogger< sirs_neuron >;

  // ----------------------------------------------------------------

  /**
   * Independent parameters of the model.
   */
  struct Parameters_
  {
    //! mean inter-update interval in ms (acts like a membrane time constant).
    double tau_m_;
    //! transition probability S->I
    double beta_sirs_;
    //! transition probability I->R
    double mu_sirs_;


    Parameters_(); //!< Sets default parameter values

    void get( DictionaryDatum& ) const;             //!< Store current values in dictionary
    void set( const DictionaryDatum&, Node* node ); //!< Set values from dicitonary
  };

  // ----------------------------------------------------------------

  /**
   * State variables of the model.
   */
  struct State_
  {
    int y_;                 //!< output of neuron in [0,1,2]
    double h_;               //!< total input current to neuron
    double last_in_node_id_; //!< node ID of the last spike being received
    Time t_next_;            //!< time point of next update
    Time t_last_in_spike_;   //!< time point of last input spike seen

    State_(); //!< Default initialization

    void get( DictionaryDatum&, const Parameters_& ) const;
    void set( const DictionaryDatum&, Node* );
  };

  // ----------------------------------------------------------------

  /**
   * Buffers of the model.
   */
  struct Buffers_
  {
    Buffers_( sirs_neuron& );
    Buffers_( const Buffers_&, sirs_neuron& );

    /** buffers and sums up incoming spikes/currents */
    RingBuffer spikes_;
    RingBuffer currents_;


    //! Logger for all analog data
    UniversalDataLogger< sirs_neuron > logger_;
  };

  // ----------------------------------------------------------------

  /**
   * Internal variables of the model.
   */
  struct Variables_
  {
    RngPtr rng_;                        //!< random number generator of my own thread
  };

  // Access functions for UniversalDataLogger -------------------------------

  //! Read out the sirs_neuron state of the neuron
  double
  get_output_state__() const
  {
    return S_.y_;
  }

  //! Read out the summed input of the neuron (= membrane potential)
  double
  get_input__() const
  {
    return S_.h_;
  }

  // ----------------------------------------------------------------

  /**
   * @defgroup iaf_psc_alpha_data
   * Instances of private data structures for the different types
   * of data pertaining to the model.
   * @note The order of definitions is important for speed.
   * @{
   */
  Parameters_ P_;
  State_ S_;
  Variables_ V_;
  Buffers_ B_;
  /** @} */

  //! Mapping of recordables names to access functions
  static RecordablesMap< sirs_neuron > recordablesMap_;
};

template <>
void RecordablesMap< sirs_neuron >::create();

} // namespace

#endif /* #ifndef SIRS_NEURON_H */