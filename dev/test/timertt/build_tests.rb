require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

#	required_prj 'test/timertt/basic_checks/prj.ut.rb'

	required_prj 'test/timertt/schedule_erase_benchmark/prj.rb'
	required_prj 'test/timertt/same_time_demands_benchmark/prj.rb'
}
