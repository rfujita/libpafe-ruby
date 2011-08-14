# -*- coding: utf-8 -*-
# $Id: suica.rb,v 1.7 2008-02-17 04:49:57 hito Exp $
require "pasori"

class Suica
  Type1 = {
    0x03 => '      精算機',
    0x05 => '    車載端末',
    0x07 => '      券売機',
    0x08 => '      券売機',
    0x09 => '      入金機',
    0x12 => '      券売機',
    0x14 => '    券売機等',
    0x15 => '    券売機等',
    0x16 => '      改札機',
    0x17 => '  簡易改札機',
    0x18 => '    窓口端末',
    0x19 => '    窓口端末',
    0x1A => '    改札端末',
    0x1B => '    携帯電話',
    0x1C => '  乗継精算機',
    0x1D => '  連絡改札機',
    0x1F => '  簡易入金機',
    0x23 => '新幹線改札機',
    0x46 => '  VIEW ALTTE',
    0x48 => '  VIEW ALTTE',
    0xC7 => '    物販端末',
    0xC8 => '      自販機',
  }

  Type2 = {
    0x01 => '              改札出場',
    0x02 => '              チャージ',
    0x03 => '            磁気券購入',
    0x04 => '                  精算',
    0x05 => '              入場精算',
    0x06 => '          改札窓口処理',
    0x07 => '              新規発行',
    0x08 => '              窓口控除',
    0x0d => '                  バス',
    0x0f => '                  バス',
    0x11 => '            再発行処理',
    0x13 => '      支払(新幹線利用)',
    0x14 => '  入場時オートチャージ',
    0x15 => '  出場時オートチャージ',
    0x1f => '          バスチャージ',
    0x23 => 'バス路面電車企画券購入',
    0x46 => '                  物販',
    0x48 => '          特典チャージ',
    0x49 => '              レジ入金',
    0x4a => '              物販取消',
    0x4b => '              入場物販',
    0xc6 => '          現金併用物販',
    0xcb => '      入場現金併用物販',
    0x84 => '              他社精算',
    0x85 => '          他社入場精算',
  }

  def initialize
    @pasori = Pasori.open
    @felica = @pasori.felica_polling(Felica::POLLING_SUICA)
  end

  def close
    @felica.close
    @pasori.close
  end

  def each(&block)
    @felica.foreach(Felica::SERVICE_SUICA_HISTORY) {|l|
      h = parse_history(l)
      yield(h)
    }
  end

  def check_val(hash, val)
    v = hash[val]
    if (v)
      v
    else
      sprintf("不明(%02x)", val)
    end
  end

  def read_in_out(&b)
    @felica.foreach(Felica::SERVICE_SUICA_IN_OUT) {|l|
      yield(l)
    }
  end

  def parse_history(l)
    d = l.unpack('CCnnCCCCvN')
    h = {}
    h["type1"] = check_val(Type1, d[0])
    h["type2"] = check_val(Type2, d[1])
    h["type3"] = d[2]
    y = (d[3] >> 9) + 2000
    m = (d[3] >> 5) & 0b1111
    dd = d[3] & 0b11111
    begin
      h["date"] = Time.local(y, m, dd)
    rescue
      return nil
    end
    h["from"] = sprintf("%02X-%02X", d[4], d[5])
    h["to"] = sprintf("%02X-%02X", d[6], d[7])
    h["balance"] = d[8]
    h["special"] = d[9]
    h
  end
end

Type3 = {
  0x2000 => '出場',
  0x4000 => '出定',
  0xa000 => '入場',
  0xc000 => '入定',
  0x0040 => '清算',
}



suica = Suica.new

i = 0
suica.read_in_out {|l|
  d = l.unpack('nCCCCnnvN')

  y = (d[5] >> 9) + 2000
  m = (d[5] >> 5) & 0x0f
  dd = d[5] & 0x1f

  printf("%s %02X-%02X %04d/%02d/%02d %02x:%02x %5d\n", Type3[d[0]], d[2], d[3], y, m, dd, d[6] >> 8, d[6] & 0xff, d[7])
}

suica.each {|h|
  printf("%s %s %04X %s %s%4s %s%4s %5d %08X\n",
         h["type1"],
         h["type2"],
         h["type3"],
         h["date"].strftime("%Y/%m/%d"),
         h["from"],
         if (h["type3"] == 3) then "(定)" else "" end,
         h["to"],
         if (h["type3"] == 4) then "(定)" else "" end,
         h["balance"],
         h["special"]
         )
}

suica.close

