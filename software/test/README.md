## __Testing__

### __Quick Start__
Plug the device to be used for testing into the GPIO Pins.

To run parallel EEPROM tests:
```sh
make EEPROM_MODEL={EEPROM_MODEL} test
```

To run I2C EEPROM tests:
```sh
make EEPROM_MODEL={EEPROM_MODEL} test_I2C
```

Where {EEPROM_MODEL} is the model of EEPROM you want to test(without brackets).

### __Running Tests__

To just build the test runner:
```sh
make test_runner_piepro
```

From there the test suit can be run with:

```sh
./bin/test_runner_piepro -m EEPROM_MODEL
```
Where EEPROM_MODEL is the model of EEPROM you want to test.

By default all Unit and Functional tests are run. You can limit which set is run with the `-u` or `--unit` flag to only runs Unit Tests or the `-f` or `--functional` flag to only runs Functional Tests.

Test data is required to run the tests and can automatically be generated with the `-g` or `--generate` flag. To only generate the test data and then exit the `-go` or `--generate-only` flag may be set.

By default Parallel EEPROM tests are run. To run the I2C tests use the `-I2C` flag.

All suites and tests are run when the test runner is invoked with out the `-s N` or `--suite N` flag which will specify to only run `N` suite. The same goes for `-t N` or `--test N` flag which will only run the specified `N` test.

The final flag, `-l N` or `--limit N`, will artificially limit the size of the EEPROM to size `N`. This is useful for quickly testing a certain test on large EEPROMS.

### __Writing Tests__

The included UTest framework should be used to write tests. Suites and tests may be added in the appropriate functions with the supplied functions in `uTest.h`.
