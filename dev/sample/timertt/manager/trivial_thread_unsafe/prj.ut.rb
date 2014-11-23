require 'mxx_ru/binary_unittest'

path = 'sample/timertt/manager/trivial_thread_unsafe'
name = 'prj'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/#{name}.ut.rb",
		"#{path}/#{name}.rb" )
)
