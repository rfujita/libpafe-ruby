# $Id: suica.rb,v 1.7 2008-02-17 04:49:57 hito Exp $
require "pasori"

class Suica
  Type1 = {
    0x03 => '精算機',
    0x05 => '車載端末',
    0x07 => '券売機',
    0x08 => '券売機',
    0x12 => '券売機',
    0x16 => '改札機',
    0x17 => '簡易改札機',
    0x18 => '窓口端末',
    0x1A => '改札端末',
    0x1B => '携帯電話',
    0x1C => '乗継精算機',
    0x1D => '連絡改札機',
    0xC7 => '物販端末',
    0xC8 => '自販機',
  }

  Type2 = {
    0x01 => '運賃支払',
    0x02 => 'チャージ',
    0x03 => '券購',
    0x04 => '精算',
    0x07 => '新規',
    0x0D => 'バス',
    0x0F => 'バス',
    0x14 => 'オートチャージ',
    0x46 => '物販',
    0xC6 => '物販(現金併用)',
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
    @felica.each(Felica::SERVICE_SUICA_IN_OUT) {|l|
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
    h["from"] = sprintf("%02x-%-2x", d[4], d[5])
    h["to"] = sprintf("%02x-%02x", d[6], d[7])
    h["balance"] = d[8]
    h["special"] = d[9]
    h
  end
end

Type3 = {
  0x20 => '出場',
  0x40 => '出定',
  0xa0 => '入場',
  0xc0 => '入定',
}



suica = Suica.new

i = 0
suica.read_in_out {|l|
  d = l.unpack('CCCCCCnnSN')

  y = (d[6] >> 9) + 2000
  m = (d[6] >> 5) & 0x0f
  dd = d[6] & 0x1f

  printf("%4s %02x-%02x %04d/%02d/%02d %02x:%02x %5d\n", Type3[d[0]], d[2], d[3], y, m, dd, d[7] >> 8, d[7] & 0xff, d[8])
}

suica.each {|h|
  printf("%16s %16s %04x %s %s%4s %s%4s %5d %08x\n",
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

