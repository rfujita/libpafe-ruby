# $Id: felica_dump.rb,v 1.9 2008-02-17 04:49:57 hito Exp $
require "pasori"

class Card
  Sinfo = {
    0 => " Area Code                  ",
    4 => " Ramdom Access R/W          ",
    5 => " Random Access Read only    ",
    6 => " Cyclic Access R/W          ",
    7 => " Cyclic Access Read only    ",
    8 => " Purse (Direct)             ",
    9 => " Purse (Cashback/decrement) ",
    10 => " Purse (Decrement)          ",
    11 => " Purse (Read Only)          ",
  }

  def Card::dump_info
    card = new
    Pasori.open {|pasori|
      pasori.felica_polling {|felica|
        system = felica.request_system
        print("# system num : #{system.length}\n")
        card.dump_system_info(pasori, system)
      }
    }
  end

  def attr2str(attr)
    a = attr.attr
    s = Sinfo[a >> 1]
    s = " INVALID or UNKNOWN" unless (s)
    s += " (PROTECTED) " if (attr.protected?)

    s
  end

  def hex_dump(ary)
    ary.unpack("C*").map{|c| sprintf("%02X", c)}.join
  end

  def dump_id(felica)
    print "  IDm: ", hex_dump(felica.idm), "\n"
    print "  PMm: ", hex_dump(felica.pmm), "\n"
  end

  def dump_service(felica, service)
    printf("# %04X:%04X %s\n", service.code, service.attr, attr2str(service))
        
    unless (service.protected?)
      j = 0
      felica.foreach(service) {|l|
        printf("  %04X:%04X:%s\n", service.code, j, hex_dump(l))
        j += 1
      }
    end
  end

  def dump_system_info(pasori, system)
    system.each {|s|
      printf("#\n# FELICA SYSTEM_CODE: %04X\n", s)

      pasori.felica_polling(s) {|felica|
        dump_id(felica)

        area = felica.area
        service = felica.service

        print "#  area num: #{area.length}\n"
        print "#  service num: #{service.length}\n"
        area.each_with_index {|a, i|
          printf("# AREA #%d %04X (%04X)\n", i, a.code, a.attr)
        }
        service.each {|s|
          dump_service(felica, s)
        }
      }
    }
  end
end

Card.dump_info
