#ifndef __action_descriptor_hpp__
#define __action_descriptor_hpp__

struct action {
  action_type_t type;
  const void* action_entry_point;
  parameter_mode_t parameter_mode;
  size_t value_size;

  action () { }

  action (action_type_t t,
	  const void* aep,
	  parameter_mode_t pm,
	  size_t vs) :
    type (t),
    action_entry_point (aep),
    parameter_mode (pm),
    value_size (vs)
  { }
};

struct action_descriptor : public action {
  aid_t parameter;

  action_descriptor () { }
  
  action_descriptor (const action& a,
		     aid_t p = 0) :
    action (a),
    parameter (p)
  { }
  
  inline bool
  operator== (const action_descriptor& other) const
  {
    return action_entry_point == other.action_entry_point && parameter == other.parameter;
  }
};

#endif /* __action_descriptor_hpp__ */
