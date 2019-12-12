require 'mxx_ru/binary_unittest'

path = 'test/timertt/manager_throwing_logger'
name = 'timer_heap_single'

MxxRu::setup_target(
	MxxRu::NegativeBinaryUnittestTarget.new(
		"#{path}/#{name}.ut.rb",
		"#{path}/#{name}.rb" )
)

