//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GRBTriggerManip.hpp
 * \ingroup GRB
 * \brief Useful functions for manipulating GRBTrigger
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: jperkins $
 * $Date: 2007/01/30 21:39:12 $
 * $Revision: 1.1 $
 * $Tag$
 *
 **/

#ifndef GRBTRIGGERMANIP_HPP
#define GRBTRIGGERMANIP_HPP

#include<stdint.h>

#include<NET_VGRBMonitor_data.h>

class GRBTriggerManip
{
public:

  static GRBTrigger* copy(const GRBTrigger* o)
  {
    GRBTrigger* c = alloc();
    c->veritas_unique_sequence_number   = o->veritas_unique_sequence_number;
    c->veritas_receipt_mjd_int          = o->veritas_receipt_mjd_int;
    c->veritas_receipt_msec_of_day_int  = o->veritas_receipt_msec_of_day_int;
    c->trigger_gcn_sequence_number      = o->trigger_gcn_sequence_number;
    c->trigger_instrument               = o->trigger_instrument;
    c->trigger_type                     = o->trigger_type;
    c->trigger_time_mjd_int             = o->trigger_time_mjd_int;
    c->trigger_msec_of_day_int          = o->trigger_msec_of_day_int;
    c->coord_ra_deg                     = o->coord_ra_deg;
    c->coord_dec_deg                    = o->coord_dec_deg;
    c->coord_epoch_J                    = o->coord_epoch_J;
    c->coord_error_circle_deg           = o->coord_error_circle_deg;
    c->veritas_should_observe           = o->veritas_should_observe;
    c->veritas_observation_window_hours = o->veritas_observation_window_hours;
    return c;
  }

  static void free(GRBTrigger* x)
  {
    //CORBA::string_free(x->trigger_instrument);
    //CORBA::string_free(x->trigger_type);
    delete x;
  }

  static GRBTrigger* alloc()
  {
    return new GRBTrigger;
  }

  static GRBTrigger* allocZero()
  {
    GRBTrigger* n = alloc();
    n->veritas_unique_sequence_number   = 0;
    n->veritas_receipt_mjd_int          = 0;
    n->veritas_receipt_msec_of_day_int  = 0;
    n->trigger_gcn_sequence_number      = 0;
    n->trigger_instrument               = static_cast<const char*>(0);
    n->trigger_type                     = static_cast<const char*>(0);
    n->trigger_time_mjd_int             = 0;
    n->trigger_msec_of_day_int          = 0;
    n->coord_ra_deg                     = 0;
    n->coord_dec_deg                    = 0;
    n->coord_epoch_J                    = 0;
    n->coord_error_circle_deg           = 0;
    n->veritas_should_observe           = 0;
    n->veritas_observation_window_hours = 0;
    return n;
  }

};

#endif // defined GRBTRIGGERMANIP_HPP
