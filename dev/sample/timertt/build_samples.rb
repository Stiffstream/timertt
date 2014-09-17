require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	required_prj 'sample/timertt/list/trivial/prj.ut.rb'

	required_prj 'sample/timertt/wheel/trivial/prj.ut.rb'

	required_prj 'sample/timertt/heap/trivial/prj.ut.rb'

	required_prj 'sample/timertt/custom_handler_and_logger/prj.rb'
}
