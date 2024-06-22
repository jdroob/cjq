.value
| if . then .error | halt_error(1)
  else null | halt_error(0)
  end