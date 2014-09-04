require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {
	required_prj 'test/timertt/build_tests.rb'

	required_prj 'sample/timertt/build_samples.rb'
}
