require 'mxx_ru/binary_unittest'

Mxx_ru::setup_target(
	Mxx_ru::Binary_unittest_target.new(
		'test/timertt/timer_heap/prj.ut.rb',
		'test/timertt/timer_heap/prj.rb' )
)

