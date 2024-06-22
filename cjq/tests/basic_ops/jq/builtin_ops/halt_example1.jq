.value
| if . then .error | halt
  else null | halt
  end