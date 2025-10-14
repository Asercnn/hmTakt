import { preferencesUtil } from "pre_as"

@Entry
@Component
struct Index {
  async aboutToAppear(): Promise<void> {
    const a = await preferencesUtil.getPreferenceValue("aaa","666","value")
    console.log(`a:${JSON.stringify(a)}`)
  }

  @State message: string = 'Hello World';

  build() {
    Row() {
      Column() {
        Text(this.message)
          .fontSize(50)
          .fontWeight(FontWeight.Bold)
      }
      .onClick(()=>{
        preferencesUtil.putPreferenceValue("aaa","777","value")
      })
      .width('100%')
    }
    .height('100%')
  }
}