#include <algorithm>
#include <vector>

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(test_energy, "Log category for SimulationTimestampEnergyTest");


class SimulationTimestampEnergyTest: public ::testing::Test {
public:
    std::unique_ptr<wrench::Workflow> workflow;

    void do_SimulationTimestampPstateSet_test();
    void do_SimulationTimestampEnergyConsumption_test();

    void do_EnergyMeterSingleMeasurementPeriod_test();
    void do_EnergyMeterMultipleMeasurementPeriod_test();

protected:

    SimulationTimestampEnergyTest() : workflow(std::unique_ptr<wrench::Workflow>(new wrench::Workflow())) {
        // Create a two-host 1-core platform file along with different pstates
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\">"
                          "<zone id=\"AS0\" routing=\"Full\">"

                          "<host id=\"host1\" speed=\"100.0Mf,50.0Mf,20.0Mf\" pstate=\"0\" core=\"1\" >"
                          "<prop id=\"wattage_per_state\" value=\"100.0:200.0, 93.0:170.0, 90.0:150.0\" />"
                          "<prop id=\"wattage_off\" value=\"10B\" />"
                          "</host>"

                          "<host id=\"host2\" speed=\"100.0Mf,50.0Mf,20.0Mf\" pstate=\"0\" core=\"1\" >"
                          "<prop id=\"wattage_per_state\" value=\"100.0:200.0, 93.0:170.0, 90.0:150.0\" />"
                          "<prop id=\"wattage_off\" value=\"10B\" />"
                          "</host>"

                          "</zone>"
                          "</platform>";

        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";

};


/**********************************************************************/
/**            SimulationTimestampPstateSetTest                      **/
/**********************************************************************/

/*
 * Testing the basic functionality of the SimulationTimestampPstateSet class.
 * This test assures that whenever the energy plugin is activated, and
 * setPstate() is called, a timestamp is recorded and added to the simulation
 * output.
 */
class SimulationTimestampPstateSetTestWMS : public wrench::WMS {
public:
    SimulationTimestampPstateSetTestWMS(SimulationTimestampEnergyTest *test,
            std::string &hostname) :
                wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    SimulationTimestampEnergyTest *test;

    int main() {
        // default ptstate = 0
        wrench::S4U_Simulation::sleep(10.000001);

        // set the pstate of the host to be 2, then 1 at the same simulated time
        // the first call to setPstate(hostname, 2) should record a timestamp
        // then the second call to setPstate(hostname, 1) should record a timestamp that replaces the one previously added
        // so that we don't end up with timestamps that show two different pstates at the same point in time
        this->simulation->setPstate(this->getHostname(), 2);
        this->simulation->setPstate(this->getHostname(), 1);

        wrench::S4U_Simulation::sleep(10.0);
        this->simulation->setPstate(this->getHostname(), 2);

        return 0;
    }

};

TEST_F(SimulationTimestampEnergyTest, SimulationTimestampPstateSetTest) {
    DO_TEST_WITH_FORK(do_SimulationTimestampPstateSet_test);
}

void SimulationTimestampEnergyTest::do_SimulationTimestampPstateSet_test() {
    auto simulation = new wrench::Simulation();
    int argc = 2;
    auto argv = (char **)calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-energy-simulation");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // get the single host
    std::string host = wrench::Simulation::getHostnameList()[0];

    std::shared_ptr<wrench::WMS> wms = nullptr;;
    EXPECT_NO_THROW(wms = simulation->add(
            new SimulationTimestampPstateSetTestWMS(
                    this, host
                    )
            ));

    EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

    EXPECT_NO_THROW(simulation->launch());

    // Check constructor for SimulationTimestampPstateSet timestamps
    EXPECT_THROW(simulation->getOutput().addTimestampPstateSet(wrench::Simulation::getCurrentSimulatedDate(), "", 0), std::invalid_argument);

    // Check that the expected SimulationTimestampPstateSet timestamps have been added to simulation output
    auto set_pstate_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampPstateSet>();
    ASSERT_EQ(4, set_pstate_timestamps.size());

    // pstate timestamp added at time 0.0
    auto ts1 = set_pstate_timestamps[0];
    ASSERT_DOUBLE_EQ(0, ts1->getDate());
    ASSERT_EQ(0, ts1->getContent()->getPstate());
    ASSERT_EQ("host1", ts1->getContent()->getHostname());

    // pstate timestamp added at time 10.000001
    auto ts2 = set_pstate_timestamps[2];
    ASSERT_DOUBLE_EQ(10.000001, ts2->getDate());
    ASSERT_EQ(1, ts2->getContent()->getPstate());
    ASSERT_EQ("host1", ts2->getContent()->getHostname());

    // pstate timestamp added at time 20.000001
    auto ts3 = set_pstate_timestamps[3];
    ASSERT_DOUBLE_EQ(20.000001, ts3->getDate());
    ASSERT_EQ(2, ts3->getContent()->getPstate());
    ASSERT_EQ("host1", ts3->getContent()->getHostname());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**            SimulationTimestampEnergyConsumption                  **/
/**********************************************************************/

/*
 * Testing the basic functionality of the SimulationTimestampEnergyConsumption class.
 * This test assures that the timestamps are added to simulation output when
 * getEnergyTimestamp("hostname", true) is called.
 */
class SimulationTimestampEnergyConsumptionTestWMS : public wrench::WMS {
public:
    SimulationTimestampEnergyConsumptionTestWMS(SimulationTimestampEnergyTest *test,
            std::string &hostname) :
                wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    SimulationTimestampEnergyTest *test;

    int main() {
        const double MEGAFLOP = 1000.0 * 1000.0;
        wrench::S4U_Simulation::compute(100.0 * MEGAFLOP); // compute for 1 second
        auto c1 = this->simulation->getEnergyConsumed(this->getHostname(), true); // 200 joules
        wrench::S4U_Simulation::compute(100.0 * MEGAFLOP); // compute for 1 second
        auto c2 = this->simulation->getEnergyConsumed(this->getHostname(), true); // now 400 joules

        // following two calls should not add any timestamps
        this->simulation->getEnergyConsumed(this->getHostname());
        this->simulation->getEnergyConsumed(this->getHostname(), false);

        return 0;
    }
};

TEST_F(SimulationTimestampEnergyTest, SimulationTimestampEnergyConsumptionTest) {
    DO_TEST_WITH_FORK(do_SimulationTimestampEnergyConsumption_test);
}

void SimulationTimestampEnergyTest::do_SimulationTimestampEnergyConsumption_test() {
    auto simulation = new wrench::Simulation();
    int argc = 2;
    auto argv = (char **)calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-energy-simulation");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // get the single host
    std::string host = wrench::Simulation::getHostnameList()[0];

    std::shared_ptr<wrench::WMS> wms = nullptr;;
    EXPECT_NO_THROW(wms = simulation->add(
            new SimulationTimestampEnergyConsumptionTestWMS(
                    this, host
            )
    ));

    EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

    EXPECT_NO_THROW(simulation->launch());

    // Check constructor for SimulationTimestampEnergyConsumption timestamp
    EXPECT_THROW(simulation->getOutput().addTimestampEnergyConsumption(0.0,"", 1.0), std::invalid_argument);

    // Check that the expected SimulationTimestampEnergyConsumption timestamps have been added to simulation output
    auto energy_consumption_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampEnergyConsumption>();
    ASSERT_EQ(2, energy_consumption_timestamps.size());

    // timestamp added after first call to getEnergyTimestamp()
    auto ts1 = energy_consumption_timestamps[0];
    ASSERT_DOUBLE_EQ(1.0, ts1->getDate());
    ASSERT_EQ("host1", ts1->getContent()->getHostname());
    ASSERT_DOUBLE_EQ(200.0, ts1->getContent()->getConsumption());

    // timestamp added after second call to getEnergyTimestamp()
    auto ts2 = energy_consumption_timestamps[1];
    ASSERT_DOUBLE_EQ(2.0, ts2->getDate());
    ASSERT_EQ("host1", ts2->getContent()->getHostname());
    ASSERT_DOUBLE_EQ(400.0, ts2->getContent()->getConsumption());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**            ENERGY METER SINGLE MEASUREMENT PERIOD TEST           **/
/**********************************************************************/

/*
 * Testing the basic functionality of the EnergyMeter class. Ensures that
 * SimulationTimestampEnergyConsumption timestamps are created for a vector
 * of hosts, all with the same measurement period.
 */
class EnergyMeterTestWMS : public wrench::WMS {
public:
    EnergyMeterTestWMS(SimulationTimestampEnergyTest *test,
            std::string &hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    SimulationTimestampEnergyTest *test;

    int main() {
        // EnergyMeter constructor tests
        try {
            auto fail_em1 = this->createEnergyMeter(std::vector<std::string>(), 1.0);
            throw std::runtime_error("createEnergyMeter should have thrown invalid argument if given an empty hostname list");
        } catch (std::invalid_argument &e) { }

        try {
            auto fail_em2 = this->createEnergyMeter({"a", "b", "c"}, 0.0);
            throw std::runtime_error("createEnergyMeter should have thrown invalid argument if given a measurement period less than 1.0");

        } catch(std::invalid_argument &e) { }

        try {
            auto fail_em3 = this->createEnergyMeter(std::map<std::string, double>());
            throw std::runtime_error("createEnergyMeter requires a non-empty map of (hostname, measurement period) pairs");
        } catch(std::invalid_argument &e) { }

        try {
            std::map<std::string, double> mp = {{"host1", 0.0}};
            auto fail_em4 = this->createEnergyMeter(mp);
            throw std::runtime_error("createEnergyMeter requires that measurement periods be 1.0 or greater");
        } catch(std::invalid_argument &e) { }

        try {
            std::map<std::string, double> mp = {{"bogus", 0.0}};
            auto fail_em5 = this->createEnergyMeter(mp);
            throw std::runtime_error("createEnergyMeter requires that hosts exist");
        } catch(std::invalid_argument &e) { }

        try {
            std::vector<std::string> mp = {{"bogus"}};
            auto fail_em6 = this->createEnergyMeter(mp, 1.0);
            throw std::runtime_error("createEnergyMeter requires that hosts exist");
        } catch(std::invalid_argument &e) { }


        // EnergyMeter functionality tests
        const std::vector<std::string> hostnames = wrench::Simulation::getHostnameList();
        const double TEN_SECOND_PERIOD = 10.0;

        auto em = this->createEnergyMeter(hostnames, TEN_SECOND_PERIOD);

        const double MEGAFLOP = 1000.0 * 1000.0;
        wrench::S4U_Simulation::compute(100.0 * 100.0 * MEGAFLOP); // compute for 100 seconds

        // Sleep 1 second to avoid having the power meters dying right when
        // The WMS is dying to, i.e., right when the simulation is terminating.
        wrench::Simulation::sleep(1.0);

        em->stop(false);

        return 0;
    }
};

TEST_F(SimulationTimestampEnergyTest, EnergyMeterSingleMeasurementPeriodTest) {
    DO_TEST_WITH_FORK(do_EnergyMeterSingleMeasurementPeriod_test);
}

void SimulationTimestampEnergyTest::do_EnergyMeterSingleMeasurementPeriod_test() {
    auto simulation = new wrench::Simulation();
    int argc = 2;
    auto argv = (char **)calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-energy-simulation");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // get the single host
    std::string host = wrench::Simulation::getHostnameList()[0];

    std::shared_ptr<wrench::WMS> wms = nullptr;;
    EXPECT_NO_THROW(wms = simulation->add(
            new EnergyMeterTestWMS(
                    this, host
            )
    ));

    EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

    EXPECT_NO_THROW(simulation->launch());

    auto energy_consumption_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampEnergyConsumption>();

    // test values for timestamps associated with host1
    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampEnergyConsumption> *> host1_timestamps;
    std::copy_if(
            energy_consumption_timestamps.begin(),
            energy_consumption_timestamps.end(),
            std::back_inserter(host1_timestamps),
            [](wrench::SimulationTimestamp<wrench::SimulationTimestampEnergyConsumption> *ts) -> bool {
        return (ts->getContent()->getHostname() == "host1" ? true : false);
    });

    // host1 computes for 100 seconds, and records timestamps in 10 second intervals, so
    // we should end up with 11 timestamps (starting with time 0)
    ASSERT_EQ(11, host1_timestamps.size());

    // expected values (timestamp, consumption)
    std::vector<std::pair<double, double>> host1_expected_timestamps = {
            {0,    0},
            {10,   2000},
            {20,   4000},
            {30,   6000},
            {40,   8000},
            {50,   10000},
            {60,   12000},
            {70,   14000},
            {80,   16000},
            {90,   18000},
            {100,  20000}
    };

    for (size_t i = 0; i < host1_timestamps.size(); ++i) {
        ASSERT_DOUBLE_EQ(host1_expected_timestamps[i].first, host1_timestamps[i]->getDate());
        ASSERT_DOUBLE_EQ(host1_expected_timestamps[i].second, host1_timestamps[i]->getContent()->getConsumption());
    }

    // test values for timestamps associated with host2
    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampEnergyConsumption> *> host2_timestamps;
    std::copy_if(
            energy_consumption_timestamps.begin(),
            energy_consumption_timestamps.end(),
            std::back_inserter(host2_timestamps),
            [](wrench::SimulationTimestamp<wrench::SimulationTimestampEnergyConsumption> *ts) -> bool {
                return (ts->getContent()->getHostname() == "host2" ? true : false);
            });

    // host2 is idle for 100 seconds, and records timestamps in 10 second intervals, so
    // we should end up with 10 timestamps (starting with time 0)
    ASSERT_EQ(11, host2_timestamps.size());

    // expected values (timestamp, consumption)
    std::vector<std::pair<double, double>> host2_expected_timestamps = {
            {0,    0},
            {10,   1000},
            {20,   2000},
            {30,   3000},
            {40,   4000},
            {50,   5000},
            {60,   6000},
            {70,   7000},
            {80,   8000},
            {90,   9000},
            {100,   10000}
    };

    for (size_t i = 0; i < host2_timestamps.size(); ++i) {
        ASSERT_DOUBLE_EQ(host2_expected_timestamps[i].first, host2_timestamps[i]->getDate());
        ASSERT_DOUBLE_EQ(host2_expected_timestamps[i].second, host2_timestamps[i]->getContent()->getConsumption());
    }

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**            ENERGY METER MULTIPLE MEASUREMENT PERIOD TEST         **/
/**********************************************************************/

/**
 * Testing the EnergyMeter class to ensure that it correctly
 * records SimulationTimestampEnergyConsumption timestamps when given
 * a map of (hostname, measurement_period) pairs.
 */

class EnergyMeterMultipleMeasurementPeriodTestWMS : public wrench::WMS {
public:
    EnergyMeterMultipleMeasurementPeriodTestWMS(SimulationTimestampEnergyTest *test,
                                                std::string &hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    SimulationTimestampEnergyTest *test;

    int main() {

        const std::map<std::string, double> measurement_periods = {
                {"host1", 2.0},
                {"host2", 3.0}
        };

        auto em = this->createEnergyMeter(measurement_periods);

        const double MEGAFLOP = 1000.0 * 1000.0;
        wrench::S4U_Simulation::compute(6.0 * 100.0 * MEGAFLOP); // compute for 6 seconds

        return 0;
    }
};

TEST_F(SimulationTimestampEnergyTest, EnergyMeterMultipleMeasurmentPeriodTest) {
    DO_TEST_WITH_FORK(do_EnergyMeterMultipleMeasurementPeriod_test);
}

void SimulationTimestampEnergyTest::do_EnergyMeterMultipleMeasurementPeriod_test() {
    auto simulation = new wrench::Simulation();
    int argc = 2;
    auto argv = (char **)calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-energy-simulation");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // get the single host
    std::string host = wrench::Simulation::getHostnameList()[0];

    std::shared_ptr<wrench::WMS> wms = nullptr;;
    EXPECT_NO_THROW(wms = simulation->add(
            new EnergyMeterMultipleMeasurementPeriodTestWMS(
                    this, host
            )
    ));

    EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

    EXPECT_NO_THROW(simulation->launch());

    auto energy_consumption_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampEnergyConsumption>();

    // test values for timestamps associated with host1
    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampEnergyConsumption> *> host1_timestamps;
    std::copy_if(
            energy_consumption_timestamps.begin(),
            energy_consumption_timestamps.end(),
            std::back_inserter(host1_timestamps),
            [](wrench::SimulationTimestamp<wrench::SimulationTimestampEnergyConsumption> *ts) -> bool {
                return (ts->getContent()->getHostname() == "host1" ? true : false);
            });

    // host1 computes for 6 seconds and records timestamps in 2.0 second intervals
    // we should end up with 4 timestamps
    ASSERT_EQ(4, host1_timestamps.size());

    // expected values (timestamp, consumption)
    std::vector<std::pair<double, double>> host1_expected_timestamps = {
            {0.0, 0.0},
            {2.0, 400.0},
            {4.0, 800.0},
            {6.0, 1200.0}
    };

    for (size_t i = 0; i < host1_timestamps.size(); ++i) {
        ASSERT_DOUBLE_EQ(host1_expected_timestamps[i].first, host1_timestamps[i]->getDate());
        ASSERT_DOUBLE_EQ(host1_expected_timestamps[i].second, host1_timestamps[i]->getContent()->getConsumption());
    }

    // test values for timestamps associated with host2
    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampEnergyConsumption> *> host2_timestamps;
    std::copy_if(
            energy_consumption_timestamps.begin(),
            energy_consumption_timestamps.end(),
            std::back_inserter(host2_timestamps),
            [](wrench::SimulationTimestamp<wrench::SimulationTimestampEnergyConsumption> *ts) -> bool {
                return (ts->getContent()->getHostname() == "host2" ? true : false);
            });

    // host2 is idle for 6 seconds and records timestamps in 3.0 second intervals
    // we should end up with 2 timestamps
    ASSERT_EQ(2, host2_timestamps.size());

    // expected values (timestamp, consumption)
    std::vector<std::pair<double, double>> host2_expected_timestamps = {
            {0.0, 0.0},
            {3.0, 300.0}
//            {6.0, 600.0}
    };

    for (size_t i = 0; i < host2_timestamps.size(); ++i) {
        ASSERT_DOUBLE_EQ(host2_expected_timestamps[i].first, host2_timestamps[i]->getDate());
        ASSERT_DOUBLE_EQ(host2_expected_timestamps[i].second, host2_timestamps[i]->getContent()->getConsumption());
    }

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
