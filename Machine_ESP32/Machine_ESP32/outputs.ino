// callback function triggered by Machine class to update 1-64 "section" outputs only
void updateSectionOutputs()
{
  Serial.print("\r\n*** Section Outputs update! *** ");
}


// callback function triggered by Machine class to update "machine" outputs
// this updates the Machine Module Pin Configuration outputs
// - sections 1-16, Hyd Up/Down, Tramline Right/Left, Geo Stop
void updateMachineOutputs()
{
  Serial.print("\r\nMachine Outputs update! ");

  for (uint8_t i = 1; i <= numMachineOutputs; i++) {
    /*Serial.print("\r\n- Pin ");
    Serial.print((machineOutputPins[i] < 10 ? " " : ""));
    Serial.print(machineOutputPins[i]); Serial.print(": ");
    Serial.print(machine.state.functions[machine.config.pinFunction[i - 1]]);
    Serial.print(" ");
    Serial.print(machine.functionNames[machine.config.pinFunction[i - 1]]);*/

    digitalWrite(machineOutputPins[i], machine.state.functions[machine.config.pinFunction[i]] == machine.config.isPinActiveHigh); // == does a XOR bit operation
  }
}

void setOutputPinModes() {
  if (numMachineOutputs > 0) {
    for (uint8_t i = 0; i < numMachineOutputs; i++) {
      pinMode(machineOutputPins[i], OUTPUT);
      digitalWrite(machineOutputPins[i], !machine.config.isPinActiveHigh);  // set OFF
    }
  }
}