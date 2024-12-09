def log_to_lab(dr_name, cycle):
  dr_group = trick.DRBinary(dr_name)
  dr_group.thisown = 0

  dr_group.set_cycle(cycle)

  hk_data = ['HkTlm.Payload.CommandCounter']

  for hk_var in hk_data:
     sbuff = 'to_lab.{0}'.format(hk_var)
     dr_group.add_variable(sbuff)

  trick.add_data_record_group(dr_group, trick.DR_Buffer)
  return dr_group
