require 'mxx_ru/binary_unittest'

path = 'test/timertt/manager_basic_checks'
name = 'timer_heap_single'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/#{name}.ut.rb",
		"#{path}/#{name}.rb" )
)

